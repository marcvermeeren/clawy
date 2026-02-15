// Clawy — JRPG companion for Claude Code sessions
// Animated pixel art fox/cat on M5StickC Plus 2 (135x240 color TFT)
// Driven by Claude Code hooks over WiFi

#define FIRMWARE_VERSION "0.1.0-beta"

#include <M5Unified.h>
#include "display.h"
#include "wifi_provision.h"
#include "wifi_transport.h"

// ============================================================
// Configuration
// ============================================================

#define FRAME_MS       200  // ~5 FPS animation
#define FRAME_MS_SLEEP 500  // ~2 FPS sleeping (slower, dreamy)
#define BUF_SIZE      256   // serial line buffer (room for MESSAGE: payloads)
#define BUF_TIMEOUT_MS 5000 // discard incomplete serial line after 5s
#define IDLE_SLEEP_MS 30000 // idle → sleeping after 30s
#define BLINK_INTERVAL 4000 // blink every ~4 seconds

#define DIM_TIMEOUT_MS   15000  // dim after 15s no state change
#define BRIGHTNESS_ACTIVE 80
#define BRIGHTNESS_DIM    20

// ============================================================
// State
// ============================================================

static char currentStatus[16] = "READY";
static char toolLabel[24] = "";
static unsigned long sessionStart = 0;
static unsigned long statusStart = 0;
static unsigned long lastFrame = 0;
static unsigned long lastBlink = 0;
static uint8_t frame = 0;
static uint8_t transitionFrames = 0;
static bool isSleeping = false;

// Boot sequence
static bool bootComplete = false;
static unsigned long bootStart = 0;

// Demo mode
static bool demoMode = false;
static uint8_t demoState = 0;
static unsigned long demoLastAdvance = 0;
#define DEMO_INTERVAL_MS 2000
#define DEMO_STATE_COUNT 7

// Auto-dim
static bool isDimmed = false;
static unsigned long lastStateChange = 0;

// Stats
static Stats stats = {0, 0, 0, 0, 0, 0};
static unsigned long workingStart = 0;  // for response time tracking
static bool showStats = false;

// Quest text (message from Claude for APPROVE/INPUT context)
static char messageText[200] = "";

// IMU easter egg
static bool imuDizzy = false;
static unsigned long dizzyStart = 0;

static char lineBuf[BUF_SIZE];
static uint8_t linePos = 0;
static unsigned long lineLastByte = 0;

static M5Canvas canvas(&M5.Display);

// ============================================================
// State helpers
// ============================================================

bool isStatus(const char* s) {
  return strcmp(currentStatus, s) == 0;
}

int getBatteryPercent() {
  int pct = M5.Power.getBatteryLevel();
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

void wakeDisplay() {
  if (isDimmed) {
    M5.Display.setBrightness(BRIGHTNESS_ACTIVE);
    isDimmed = false;
  }
  lastStateChange = millis();
}

// ============================================================
// IMU update (shake → dizzy easter egg)
// ============================================================

void updateIMU() {
  float ax, ay, az;
  M5.Imu.getAccelData(&ax, &ay, &az);

  // Shake: high acceleration triggers dizzy
  float mag = ax * ax + ay * ay + az * az;
  if (mag > 6.0f && !imuDizzy) {
    imuDizzy = true;
    dizzyStart = millis();
  }
  if (imuDizzy && millis() - dizzyStart > 2000) {
    imuDizzy = false;
  }
}

// ============================================================
// Status text for display
// ============================================================

const char* getStatusLabel() {
  if (isSleeping) return "Sleeping...";
  if (isStatus("READY"))    return "~ Clawy ~";
  if (isStatus("WORKING"))  return "Thinking...";
  if (isStatus("TOOL"))     return toolLabel;
  if (isStatus("DONE"))     return "Done!";
  if (isStatus("INPUT"))    return "Need Input";
  if (isStatus("APPROVE"))  return "Approve?";
  if (isStatus("ERROR"))    return "Error!";
  return currentStatus;
}

const char* getDetailLabel() {
  if (isStatus("READY") && !isSleeping) return "Awaiting orders...";
  return NULL;
}

// ============================================================
// Animation frame selection
// ============================================================

const uint8_t* getCurrentSprite() {
  if (imuDizzy) {
    return (const uint8_t*)pgm_read_ptr(&dizzy_frames[frame % DIZZY_FRAME_COUNT]);
  }

  if (isSleeping) {
    return (const uint8_t*)pgm_read_ptr(&sleeping_frames[frame % SLEEPING_FRAME_COUNT]);
  }

  if (isStatus("READY")) {
    if (millis() - lastBlink < 300) {
      return sprite_idle_blink;
    }
    return (const uint8_t*)pgm_read_ptr(&idle_frames[frame % IDLE_FRAME_COUNT]);
  }
  if (isStatus("WORKING")) {
    return (const uint8_t*)pgm_read_ptr(&thinking_frames[frame % THINKING_FRAME_COUNT]);
  }
  if (isStatus("TOOL")) {
    return (const uint8_t*)pgm_read_ptr(&running_frames[frame % RUNNING_FRAME_COUNT]);
  }
  if (isStatus("DONE")) {
    return (const uint8_t*)pgm_read_ptr(&happy_frames[frame % HAPPY_FRAME_COUNT]);
  }
  if (isStatus("INPUT")) {
    return (const uint8_t*)pgm_read_ptr(&curious_frames[frame % CURIOUS_FRAME_COUNT]);
  }
  if (isStatus("APPROVE")) {
    return sprite_alert;
  }
  if (isStatus("ERROR")) {
    return (const uint8_t*)pgm_read_ptr(&dizzy_frames[frame % DIZZY_FRAME_COUNT]);
  }

  return sprite_idle_1;
}

// ============================================================
// Draw particle effects for current state
// ============================================================

void drawParticles(M5Canvas& cv, uint8_t f, uint16_t color) {
  if (imuDizzy) { drawCirclingStars(cv, f, COL_YELLOW); return; }
  if (isSleeping) {
    drawFloatingZZZ(cv, f, color);
    return;
  }
  if (isStatus("WORKING"))  drawThoughtDots(cv, f, color);
  if (isStatus("TOOL"))     drawDustPuffs(cv, f, color);
  if (isStatus("DONE"))     drawSparkles(cv, f, color);
  if (isStatus("INPUT"))    drawBouncingQuestion(cv, f, color);
  if (isStatus("APPROVE"))  drawPulsingBang(cv, f, color);
  if (isStatus("ERROR"))    drawCirclingStars(cv, f, color);
}

// ============================================================
// Render one frame
// ============================================================

void renderFrame() {
  uint16_t accent = isSleeping ? COL_DIM_GRAY : stateColor(currentStatus);
  int battPct = getBatteryPercent();

  // Screen shake offset for ERROR state
  int16_t shakeX = 0, shakeY = 0;
  if (isStatus("ERROR") && !isSleeping) {
    const int8_t shakeTable[] = {-2, 1, 2, -1, 0, 2, -1, 1};
    shakeX = shakeTable[frame % 8];
    shakeY = shakeTable[(frame + 3) % 8];
  }

  // Clear canvas
  canvas.fillSprite(COL_BLACK);

  // HUD bar
  drawHUD(canvas, sessionStart, battPct, wifiIsConnected());

  // Accent divider
  drawDivider(canvas, accent);

  // Stats screen overlay (replaces portrait + text window)
  if (showStats) {
    drawStatsScreen(canvas, stats, sessionStart, accent);
    canvas.pushSprite(0, 0);
    return;
  }

  // Portrait frame
  drawPortraitFrame(canvas, accent);

  // Background effect (inside portrait frame, behind character)
  drawBackground(canvas, currentStatus, isSleeping, frame, accent);

  // Character sprite (with shake offset for ERROR)
  const uint8_t* sprite = getCurrentSprite();
  drawSpriteTinted(canvas, sprite, CHAR_X + shakeX, CHAR_Y + shakeY, accent);

  // Particles
  drawParticles(canvas, frame, accent);

  // Text window
  drawTextWindow(canvas, accent);

  // Status text
  drawStatusText(canvas, getStatusLabel(), accent);

  // Detail line
  const char* detail = getDetailLabel();
  if (detail) {
    drawDetailText(canvas, detail, COL_HUD_GRAY);
  } else if (isStatus("WORKING") || isStatus("TOOL")) {
    unsigned long sec = (millis() - statusStart) / 1000;
    char timeBuf[8];
    if (sec < 60) {
      snprintf(timeBuf, sizeof(timeBuf), "%lus", sec);
    } else {
      snprintf(timeBuf, sizeof(timeBuf), "%lu:%02lu", sec / 60, sec % 60);
    }
    drawDetailText(canvas, timeBuf, COL_HUD_GRAY);
  }

  // Quest text scroll (APPROVE/INPUT message context)
  drawQuestScroll(canvas, messageText, accent, frame);

  // Button bar (APPROVE only)
  if (isStatus("APPROVE")) {
    drawButtonBar(canvas);
  }

  // Push to display
  canvas.pushSprite(0, 0);
}

// ============================================================
// State transitions
// ============================================================

void showStatus(const char* status) {
  if (strcmp(currentStatus, status) == 0 && !isSleeping) return;

  // Stats tracking
  if (strcmp(status, "WORKING") == 0) {
    stats.promptCount++;
    workingStart = millis();
  }
  if (strcmp(status, "DONE") == 0) {
    stats.doneCount++;
    if (workingStart > 0) {
      stats.totalResponseMs += (millis() - workingStart);
      stats.responseCount++;
      workingStart = 0;
    }
  }
  if (strcmp(status, "ERROR") == 0) {
    stats.errorCount++;
  }

  // Clear approval pending when leaving APPROVE state
  if (isStatus("APPROVE")) wifiSetApprovalPending(false);

  isSleeping = false;
  strncpy(currentStatus, status, sizeof(currentStatus) - 1);
  currentStatus[sizeof(currentStatus) - 1] = '\0';
  toolLabel[0] = '\0';
  statusStart = millis();
  frame = 0;
  lastFrame = millis();
  lastBlink = 0;
  transitionFrames = 1;

  // Close stats on any state change
  showStats = false;

  // Wake display
  wakeDisplay();

  renderFrame();
}

void showTool(const char* label) {
  bool wasAlreadyTool = isStatus("TOOL");
  strncpy(currentStatus, "TOOL", sizeof(currentStatus) - 1);
  strncpy(toolLabel, label, sizeof(toolLabel) - 1);
  toolLabel[sizeof(toolLabel) - 1] = '\0';

  // Stats tracking
  stats.toolCallCount++;

  isSleeping = false;
  if (!wasAlreadyTool) {
    statusStart = millis();
    frame = 0;
    lastFrame = millis();
    transitionFrames = 1;
  }

  showStats = false;
  wakeDisplay();
  renderFrame();
}

// ============================================================
// Demo mode
// ============================================================

void demoAdvance() {
  demoState = (demoState + 1) % DEMO_STATE_COUNT;
  demoLastAdvance = millis();
  messageText[0] = '\0';

  switch (demoState) {
    case 0: showStatus("READY");   break;
    case 1: showStatus("WORKING"); break;
    case 2: showTool("Bash");      break;
    case 3: showStatus("DONE");    break;
    case 4:
      showStatus("INPUT");
      strncpy(messageText, "What color theme?", sizeof(messageText) - 1);
      renderFrame();
      break;
    case 5:
      showStatus("APPROVE");
      strncpy(messageText, "Run: npm install", sizeof(messageText) - 1);
      renderFrame();
      break;
    case 6: showStatus("ERROR");   break;
  }
}

void demoEnter() {
  demoMode = true;
  demoState = DEMO_STATE_COUNT - 1; // will wrap to 0 on first advance
  demoAdvance();
}

void demoExit() {
  demoMode = false;
  messageText[0] = '\0';
  showStatus("READY");
}

// ============================================================
// Serial protocol
// ============================================================

void processLine(const char* line) {
  if (demoMode) {
    demoExit();
    // Fall through to process the command normally
  }

  if (strncmp(line, "STATUS:", 7) == 0) {
    const char* val = line + 7;
    // Clear quest text on every status change
    messageText[0] = '\0';
    if (strncmp(val, "TOOL:", 5) == 0) {
      showTool(val + 5);
    } else {
      showStatus(val);
      if (strcmp(val, "APPROVE") == 0) wifiSetApprovalPending(true);
    }
    Serial.printf("OK %s\n", val);
  } else if (strncmp(line, "MESSAGE:", 8) == 0) {
    const char* msg = line + 8;
    strncpy(messageText, msg, sizeof(messageText) - 1);
    messageText[sizeof(messageText) - 1] = '\0';
    renderFrame();  // re-render to show quest text
  }
}

// ============================================================
// Idle progression (READY → sleeping)
// ============================================================

void checkIdleProgression() {
  unsigned long elapsed = millis() - statusStart;

  // Blink trigger (READY only)
  if (isStatus("READY") && elapsed < IDLE_SLEEP_MS && (millis() - lastBlink > BLINK_INTERVAL)) {
    lastBlink = millis();
  }

  // Sleep transition — any idle state (READY, DONE, ERROR) can sleep
  if (!isSleeping && elapsed >= IDLE_SLEEP_MS) {
    // Don't sleep during active/interactive states
    if (isStatus("WORKING") || isStatus("TOOL") || isStatus("INPUT") || isStatus("APPROVE")) return;
    isSleeping = true;
    frame = 0;
  }
}

// ============================================================
// Auto-dim
// ============================================================

void checkAutoDim() {
  if (isDimmed) return;
  if (millis() - lastStateChange >= DIM_TIMEOUT_MS) {
    M5.Display.setBrightness(BRIGHTNESS_DIM);
    isDimmed = true;
  }
}

// ============================================================
// Button handling
// ============================================================

void checkButtons() {
  // Demo mode buttons: A advances, B exits
  if (demoMode) {
    if (M5.BtnA.wasClicked()) {
      wakeDisplay();
      demoAdvance();
    }
    if (M5.BtnB.wasClicked() || M5.BtnB.wasHold()) {
      wakeDisplay();
      demoExit();
    }
    return;
  }

  // Button A: approve (only in APPROVE state)
  if (isStatus("APPROVE") && M5.BtnA.wasClicked()) {
    Serial.println("BUTTON:APPROVE");
    wifiSendApproval("BUTTON:APPROVE");
    Serial.flush();  // drain TX before CPU-heavy renderFrame()
    delay(10);       // give CH9102F bridge time to forward bytes over USB
    showStatus("WORKING");
    return;
  }

  // Button B long press: enter demo mode (except during APPROVE)
  if (!isStatus("APPROVE") && M5.BtnB.wasHold()) {
    wakeDisplay();
    demoEnter();
    return;
  }

  // Button B click: deny (in APPROVE state) OR toggle stats
  if (M5.BtnB.wasClicked()) {
    wakeDisplay();
    if (isStatus("APPROVE")) {
      Serial.println("BUTTON:DENY");
      wifiSendApproval("BUTTON:DENY");
      Serial.flush();  // drain TX before CPU-heavy renderFrame()
      delay(10);       // give CH9102F bridge time to forward bytes over USB
      showStatus("READY");
    } else {
      // Toggle stats screen
      showStats = !showStats;
      renderFrame();
    }
  }

  // Any button press wakes display
  if (M5.BtnA.wasClicked()) {
    wakeDisplay();
  }
}

// ============================================================
// Setup
// ============================================================

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);

  // Button B hold threshold for demo mode entry
  M5.BtnB.setHoldThresh(2000);

  // Display setup
  M5.Display.setRotation(0);  // Portrait
  M5.Display.fillScreen(COL_BLACK);
  M5.Display.setBrightness(BRIGHTNESS_ACTIVE);

  // Create full-screen sprite for flicker-free rendering
  canvas.createSprite(SCREEN_W, SCREEN_H);
  canvas.setSwapBytes(true);

  // Check for WiFi reset: hold both buttons during boot
  M5.update();
  if (M5.BtnA.isPressed() && M5.BtnB.isPressed()) {
    nvsClearWiFiCreds();
    // Show reset message briefly
    canvas.fillSprite(COL_BLACK);
    canvas.setTextColor(COL_ORANGE);
    canvas.setFont(&fonts::Font2);
    canvas.setTextDatum(middle_center);
    canvas.drawString("WiFi Reset", SCREEN_W / 2, SCREEN_H / 2 - 10);
    canvas.setTextColor(COL_HUD_GRAY);
    canvas.setFont(&fonts::Font0);
    canvas.drawString("Connect via browser", SCREEN_W / 2, SCREEN_H / 2 + 10);
    canvas.pushSprite(0, 0);
    delay(2000);
  }

  // WiFi: try NVS creds first, then secrets.h fallback, then provisioning mode
  char nvsSsid[33] = {0};
  char nvsPass[65] = {0};
  if (nvsLoadWiFiCreds(nvsSsid, sizeof(nvsSsid), nvsPass, sizeof(nvsPass))) {
    wifiBeginWithCreds(nvsSsid, nvsPass);
    Serial.printf("WiFi: using saved creds (%s)\n", nvsSsid);
  }
#if defined(WIFI_SSID) && defined(WIFI_PASS)
  else {
    wifiBegin();
    Serial.println("WiFi: using compile-time creds");
  }
#else
  else {
    // No creds anywhere — enter Improv provisioning mode
    // NOTE: no Serial.println here — text output corrupts the Improv binary stream
    provisioningEnter();
  }
#endif

  // Boot sequence
  bootStart = millis();
  bootComplete = false;

  // Initial state
  sessionStart = millis();
  statusStart = millis();
  lastBlink = millis();
  lastStateChange = millis();

  // Only print ready message if NOT in provisioning mode (text corrupts Improv)
  if (!provisioningIsActive()) {
    Serial.println("Clawy ready");
  }
}

// ============================================================
// Main loop
// ============================================================

void loop() {
  M5.update();

  // WiFi provisioning mode — handle serial protocol + show setup screen
  if (provisioningIsActive()) {
    provisionLoop();
    static unsigned long lastProvFrame = 0;
    if (millis() - lastProvFrame >= 500) {
      lastProvFrame = millis();
      canvas.fillSprite(COL_BLACK);
      canvas.setTextColor(COL_CYAN);
      canvas.setFont(&fonts::Font2);
      canvas.setTextDatum(middle_center);
      canvas.drawString("WiFi Setup", SCREEN_W / 2, SCREEN_H / 2 - 30);
      canvas.setTextColor(COL_HUD_GRAY);
      canvas.setFont(&fonts::Font0);
      canvas.drawString("Open browser to", SCREEN_W / 2, SCREEN_H / 2);
      canvas.drawString("configure WiFi", SCREEN_W / 2, SCREEN_H / 2 + 12);
      // Blinking dot
      if ((millis() / 500) % 2 == 0) {
        canvas.fillCircle(SCREEN_W / 2, SCREEN_H / 2 + 35, 3, COL_CYAN);
      }
      canvas.pushSprite(0, 0);
    }
    return;
  }

  // Boot sequence (plays intro, then idles on title until Button A)
  if (!bootComplete) {
    wifiCheck();
    wifiPoll();
    drawBootScreen(canvas, bootStart, wifiGetIP());
    // Button A: "PRESS START" — begin game
    if (M5.BtnA.wasClicked()) {
      bootComplete = true;
      renderFrame();
    }
    // Still read serial during boot so we don't miss commands
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (linePos > 0) {
          lineBuf[linePos] = '\0';
          processLine(lineBuf);
          linePos = 0;
          bootComplete = true;  // Skip remaining boot on first command
        }
      } else if (linePos < BUF_SIZE - 1) {
        lineBuf[linePos++] = c;
      }
    }
    return;
  }

  // Read serial
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (linePos > 0) {
        lineBuf[linePos] = '\0';
        processLine(lineBuf);
        linePos = 0;
      }
    } else if (linePos < BUF_SIZE - 1) {
      lineBuf[linePos++] = c;
      lineLastByte = millis();
    }
  }

  // Discard incomplete serial buffer after timeout
  if (linePos > 0 && millis() - lineLastByte > BUF_TIMEOUT_MS) {
    linePos = 0;
  }

  // WiFi
  wifiCheck();
  wifiPoll();

  // Button handling
  checkButtons();

  // Idle progression
  checkIdleProgression();

  // Auto-dim check
  checkAutoDim();

  // Demo mode auto-advance
  if (demoMode && millis() - demoLastAdvance >= DEMO_INTERVAL_MS) {
    demoAdvance();
  }

  // IMU easter egg (tilt + shake)
  updateIMU();

  // Animation frame update (slower when sleeping)
  unsigned long frameInterval = isSleeping ? FRAME_MS_SLEEP : FRAME_MS;
  if (millis() - lastFrame >= frameInterval) {
    lastFrame = millis();

    if (transitionFrames > 0) {
      transitionFrames--;
    } else {
      frame++;
    }

    renderFrame();
  }
}
