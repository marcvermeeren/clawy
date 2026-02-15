#pragma once
#include <M5Unified.h>
#include "sprites.h"

// ============================================================
// Display: 135x240 TFT, portrait orientation
// ============================================================

#define SCREEN_W 135
#define SCREEN_H 240

// Layout zones (JRPG portrait + text window layout)
#define HUD_Y       0
#define HUD_H       16
#define DIVIDER_Y   17

// Portrait frame area
#define FRAME_X     4
#define FRAME_Y     22
#define FRAME_W     127
#define FRAME_H     82

// Character position (centered in portrait frame)
#define CHAR_CX     67
#define CHAR_CY     (FRAME_Y + 6 + SPRITE_H / 2)
#define CHAR_X      (CHAR_CX - SPRITE_W / 2)
#define CHAR_Y      (FRAME_Y + 6)

// Text window area
#define TBOX_X      4
#define TBOX_Y      108
#define TBOX_W      127
#define TBOX_H      40

// Text positions inside text window
#define STATUS_Y    120
#define DETAIL_Y    138

// Button bar
#define BTN_Y       210
#define BTN_H       24

// ============================================================
// Color palette (RGB565)
// ============================================================

#define COL_BLACK     0x0000
#define COL_WHITE     0xFFFF
#define COL_CYAN      0x07FF
#define COL_GREEN     0x07E0
#define COL_YELLOW    0xFFE0
#define COL_MAGENTA   0xF81F
#define COL_ORANGE    0xFBE0
#define COL_RED       0xF800
#define COL_HUD_GRAY  0x4208
#define COL_DIM_GRAY  0x3186
#define COL_DARK_BG   0x0821  // very dark gray for HUD background
#define COL_PINK      0xFB56  // cheek blush for DONE state
#define COL_HEART     0xF800  // heart color
#define COL_FRAME_BG  0x0000  // portrait frame interior

// Dim an RGB565 color by shifting each channel right
uint16_t dimRGB565(uint16_t color, uint8_t shift = 1) {
  uint16_t r = (color >> 11) & 0x1F;
  uint16_t g = (color >> 5) & 0x3F;
  uint16_t b = color & 0x1F;
  r >>= shift;
  g >>= shift;
  b >>= shift;
  return (r << 11) | (g << 5) | b;
}

// Lerp between two RGB565 colors; t: 0-255
uint16_t lerpRGB565(uint16_t a, uint16_t b, uint8_t t) {
  uint16_t ra = (a >> 11) & 0x1F, ga = (a >> 5) & 0x3F, ba = a & 0x1F;
  uint16_t rb = (b >> 11) & 0x1F, gb = (b >> 5) & 0x3F, bb = b & 0x1F;
  uint16_t r = ra + (((int16_t)rb - ra) * t >> 8);
  uint16_t g = ga + (((int16_t)gb - ga) * t >> 8);
  uint16_t bl = ba + (((int16_t)bb - ba) * t >> 8);
  return (r << 11) | (g << 5) | bl;
}

// Per-star color palette (8 hues for boot starfield)
static const uint16_t starHues[] = {
  0xFFFF,  // white
  0xAEFF,  // light blue
  0xFEA0,  // warm yellow
  0xFCA0,  // amber
  0x07FF,  // cyan
  0xFCDF,  // light pink
  0xB5FF,  // pale blue
  0xF720,  // gold
};

// Per-character gradient for "Clawy" title (5 chars)
static const uint16_t titleGrad[] = {
  0x07FF,  // C — cyan
  0xFFFF,  // l — white
  0xF81F,  // a — magenta
  0xFFE0,  // w — yellow
  0xFFFF,  // y — white
};

// Extended palette for idle title color rotation (8 entries)
static const uint16_t titlePalette[] = {
  0x07FF,  // cyan
  0xFFFF,  // white
  0xF81F,  // magenta
  0xFFE0,  // yellow
  0xFFFF,  // white
  0xFB56,  // pink
  0x07E0,  // green
  0xFBE0,  // orange
};

// State accent color lookup
uint16_t stateColor(const char* status) {
  if (strcmp(status, "READY") == 0)    return COL_WHITE;
  if (strcmp(status, "WORKING") == 0)  return COL_CYAN;
  if (strcmp(status, "TOOL") == 0)     return COL_GREEN;
  if (strcmp(status, "DONE") == 0)     return COL_YELLOW;
  if (strcmp(status, "INPUT") == 0)    return COL_MAGENTA;
  if (strcmp(status, "APPROVE") == 0)  return COL_ORANGE;
  if (strcmp(status, "ERROR") == 0)    return COL_RED;
  if (strcmp(status, "SLEEPING") == 0) return COL_DIM_GRAY;
  return COL_WHITE;
}

// ============================================================
// Sprite rendering — draw 1-bit bitmap with tint color
// ============================================================

void drawSpriteTinted(M5Canvas& canvas, const uint8_t* bitmap, int16_t x, int16_t y,
                      uint16_t color) {
  const int bytesPerRow = SPRITE_W / 8;
  for (int row = 0; row < SPRITE_H; row++) {
    for (int col = 0; col < SPRITE_W; col++) {
      int byteIdx = row * bytesPerRow + (col / 8);
      int bitIdx = 7 - (col % 8);
      uint8_t b = pgm_read_byte(&bitmap[byteIdx]);
      if (b & (1 << bitIdx)) {
        canvas.drawPixel(x + col, y + row, color);
      }
    }
  }
}

// Draw 16x16 1-bit icon
void drawIcon16(M5Canvas& canvas, const uint8_t* bitmap, int16_t x, int16_t y,
                uint16_t color) {
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 16; col++) {
      int byteIdx = row * 2 + (col / 8);
      int bitIdx = 7 - (col % 8);
      uint8_t b = pgm_read_byte(&bitmap[byteIdx]);
      if (b & (1 << bitIdx)) {
        canvas.drawPixel(x + col, y + row, color);
      }
    }
  }
}

// ============================================================
// HUD bar — hearts + timer + battery
// ============================================================

void drawHeart(M5Canvas& canvas, int16_t x, int16_t y, uint16_t color) {
  canvas.drawPixel(x + 1, y, color);
  canvas.drawPixel(x + 3, y, color);
  canvas.fillRect(x, y + 1, 5, 1, color);
  canvas.fillRect(x, y + 2, 5, 1, color);
  canvas.drawPixel(x + 1, y + 3, color);
  canvas.drawPixel(x + 2, y + 3, color);
  canvas.drawPixel(x + 3, y + 3, color);
  canvas.drawPixel(x + 2, y + 4, color);
}

void drawBattery(M5Canvas& canvas, int16_t x, int16_t y, int percent) {
  canvas.drawRect(x, y + 1, 14, 7, COL_HUD_GRAY);
  canvas.fillRect(x + 14, y + 3, 2, 3, COL_HUD_GRAY);
  int fillW = (percent * 12) / 100;
  if (fillW < 0) fillW = 0;
  if (fillW > 12) fillW = 12;
  uint16_t fillColor = (percent > 20) ? COL_GREEN : COL_RED;
  if (fillW > 0) {
    canvas.fillRect(x + 1, y + 2, fillW, 5, fillColor);
  }
}

void drawHUD(M5Canvas& canvas, unsigned long sessionStart, int batteryPercent,
             bool wifiUp = false) {
  canvas.fillRect(0, HUD_Y, SCREEN_W, HUD_H, COL_DARK_BG);

  for (int i = 0; i < 5; i++) {
    drawHeart(canvas, 4 + i * 7, 5, COL_HEART);
  }

  unsigned long sec = (millis() - sessionStart) / 1000;
  char timeBuf[8];
  if (sec < 60) {
    snprintf(timeBuf, sizeof(timeBuf), "%lus", sec);
  } else if (sec < 3600) {
    snprintf(timeBuf, sizeof(timeBuf), "%lu:%02lu", sec / 60, sec % 60);
  } else {
    snprintf(timeBuf, sizeof(timeBuf), "%luh%02lu", sec / 3600, (sec % 3600) / 60);
  }
  canvas.setTextColor(COL_HUD_GRAY);
  canvas.setTextSize(1);
  canvas.setFont(&fonts::Font0);
  canvas.setTextDatum(middle_center);
  canvas.drawString(timeBuf, SCREEN_W / 2, HUD_Y + HUD_H / 2);

  // WiFi status dot (green = connected, red = disconnected)
  canvas.fillCircle(SCREEN_W - 27, 7, 2, wifiUp ? COL_GREEN : COL_RED);

  drawBattery(canvas, SCREEN_W - 22, 3, batteryPercent);
}

// ============================================================
// Divider line
// ============================================================

void drawDivider(M5Canvas& canvas, uint16_t color) {
  canvas.drawFastHLine(0, DIVIDER_Y, SCREEN_W, color);
  canvas.drawFastHLine(0, DIVIDER_Y + 1, SCREEN_W, color);
}

// ============================================================
// JRPG Portrait Frame — double-border with corner brackets
// ============================================================

void drawPortraitFrame(M5Canvas& canvas, uint16_t accent) {
  uint16_t dim = dimRGB565(accent, 1);
  uint16_t vdim = dimRGB565(accent, 2);

  // Outer border
  canvas.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, dim);
  // Inner border (1px inset)
  canvas.drawRect(FRAME_X + 2, FRAME_Y + 2, FRAME_W - 4, FRAME_H - 4, vdim);

  // Corner brackets (3px L-shapes at each corner)
  int bLen = 8;
  // Top-left
  canvas.drawFastHLine(FRAME_X, FRAME_Y, bLen, accent);
  canvas.drawFastVLine(FRAME_X, FRAME_Y, bLen, accent);
  // Top-right
  canvas.drawFastHLine(FRAME_X + FRAME_W - bLen, FRAME_Y, bLen, accent);
  canvas.drawFastVLine(FRAME_X + FRAME_W - 1, FRAME_Y, bLen, accent);
  // Bottom-left
  canvas.drawFastHLine(FRAME_X, FRAME_Y + FRAME_H - 1, bLen, accent);
  canvas.drawFastVLine(FRAME_X, FRAME_Y + FRAME_H - bLen, bLen, accent);
  // Bottom-right
  canvas.drawFastHLine(FRAME_X + FRAME_W - bLen, FRAME_Y + FRAME_H - 1, bLen, accent);
  canvas.drawFastVLine(FRAME_X + FRAME_W - 1, FRAME_Y + FRAME_H - bLen, bLen, accent);
}

// ============================================================
// JRPG Text Window — double-border dialog box
// ============================================================

void drawTextWindow(M5Canvas& canvas, uint16_t accent) {
  uint16_t dim = dimRGB565(accent, 1);
  uint16_t vdim = dimRGB565(accent, 2);

  // Outer border
  canvas.drawRect(TBOX_X, TBOX_Y, TBOX_W, TBOX_H, dim);
  // Inner border
  canvas.drawRect(TBOX_X + 2, TBOX_Y + 2, TBOX_W - 4, TBOX_H - 4, vdim);

  // Corner brackets
  int bLen = 6;
  canvas.drawFastHLine(TBOX_X, TBOX_Y, bLen, accent);
  canvas.drawFastVLine(TBOX_X, TBOX_Y, bLen, accent);
  canvas.drawFastHLine(TBOX_X + TBOX_W - bLen, TBOX_Y, bLen, accent);
  canvas.drawFastVLine(TBOX_X + TBOX_W - 1, TBOX_Y, bLen, accent);
  canvas.drawFastHLine(TBOX_X, TBOX_Y + TBOX_H - 1, bLen, accent);
  canvas.drawFastVLine(TBOX_X, TBOX_Y + TBOX_H - bLen, bLen, accent);
  canvas.drawFastHLine(TBOX_X + TBOX_W - bLen, TBOX_Y + TBOX_H - 1, bLen, accent);
  canvas.drawFastVLine(TBOX_X + TBOX_W - 1, TBOX_Y + TBOX_H - bLen, bLen, accent);
}

// ============================================================
// Status text (inside text window)
// ============================================================

void drawStatusText(M5Canvas& canvas, const char* text, uint16_t color) {
  canvas.setTextColor(color);
  canvas.setTextSize(1);
  canvas.setFont(&fonts::Font2);
  canvas.setTextDatum(middle_center);
  canvas.drawString(text, SCREEN_W / 2, STATUS_Y);
}

void drawDetailText(M5Canvas& canvas, const char* text, uint16_t color) {
  canvas.setTextColor(color);
  canvas.setTextSize(1);
  canvas.setFont(&fonts::Font0);
  canvas.setTextDatum(middle_center);
  canvas.drawString(text, SCREEN_W / 2, DETAIL_Y);
}

// ============================================================
// Button bar (APPROVE state only)
// ============================================================

void drawButtonBar(M5Canvas& canvas) {
  int btnW = 58;
  int gap = 5;
  int totalW = btnW * 2 + gap;
  int startX = (SCREEN_W - totalW) / 2;

  canvas.drawRoundRect(startX, BTN_Y, btnW, BTN_H, 4, COL_GREEN);
  canvas.setTextColor(COL_GREEN);
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  canvas.setTextDatum(middle_center);
  canvas.drawString("A: Yes!", startX + btnW / 2, BTN_Y + BTN_H / 2);

  canvas.drawRoundRect(startX + btnW + gap, BTN_Y, btnW, BTN_H, 4, COL_RED);
  canvas.setTextColor(COL_RED);
  canvas.drawString("B: Nope", startX + btnW + gap + btnW / 2, BTN_Y + BTN_H / 2);
}

// ============================================================
// Per-state background effects (inside portrait frame)
// ============================================================

// Clipping helper — only draw inside portrait frame interior
static inline bool inFrame(int16_t x, int16_t y) {
  return x >= FRAME_X + 3 && x < FRAME_X + FRAME_W - 3 &&
         y >= FRAME_Y + 3 && y < FRAME_Y + FRAME_H - 3;
}

// READY: twinkling star field
void drawBgStarfield(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  uint16_t dim = dimRGB565(color, 2);
  // Pseudo-random star positions using simple hash
  for (int i = 0; i < 12; i++) {
    int16_t x = FRAME_X + 5 + ((i * 37 + 13) % (FRAME_W - 10));
    int16_t y = FRAME_Y + 5 + ((i * 23 + 7) % (FRAME_H - 10));
    if (inFrame(x, y)) {
      // Twinkle: some stars bright, some dim, cycling
      if ((frame + i) % 5 < 3) {
        canvas.drawPixel(x, y, color);
      } else {
        canvas.drawPixel(x, y, dim);
      }
    }
  }
}

// WORKING: flowing wave (dotted horizontal lines with sine offset)
void drawBgWave(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  uint16_t dim = dimRGB565(color, 2);
  for (int row = 0; row < 4; row++) {
    int16_t y = FRAME_Y + 15 + row * 18;
    for (int col = 0; col < FRAME_W - 10; col += 4) {
      // Simple sine approximation: offset = sin LUT
      const int8_t sinLut[] = {0, 2, 3, 2, 0, -2, -3, -2};
      int phase = (col / 4 + frame + row * 2) % 8;
      int16_t x = FRAME_X + 5 + col;
      int16_t dy = y + sinLut[phase];
      if (inFrame(x, dy)) {
        canvas.drawPixel(x, dy, dim);
      }
    }
  }
}

// TOOL: speed lines (horizontal dashes scrolling left)
void drawBgSpeedLines(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  uint16_t dim = dimRGB565(color, 2);
  for (int i = 0; i < 6; i++) {
    int16_t y = FRAME_Y + 8 + i * 12;
    int offset = (frame * 3 + i * 7) % 20;
    for (int x = FRAME_X + FRAME_W - 6 - offset; x > FRAME_X + 4; x -= 20) {
      int dashLen = 5 + (i % 3) * 2;
      for (int d = 0; d < dashLen && inFrame(x - d, y); d++) {
        canvas.drawPixel(x - d, y, dim);
      }
    }
  }
}

// DONE: confetti (small rects falling/floating)
void drawBgConfetti(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  for (int i = 0; i < 8; i++) {
    int16_t x = FRAME_X + 6 + ((i * 41 + 11) % (FRAME_W - 12));
    int16_t baseY = FRAME_Y + 5 + ((i * 19 + frame * 2) % (FRAME_H - 10));
    uint16_t c = (i % 2 == 0) ? color : dimRGB565(color, 1);
    if (inFrame(x, baseY) && inFrame(x + 1, baseY + 1)) {
      canvas.fillRect(x, baseY, 2, 2, c);
    }
  }
}

// INPUT: pulsing glow (concentric circles breathing)
void drawBgPulse(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  uint16_t dim = dimRGB565(color, 3);
  int16_t cx = CHAR_CX;
  int16_t cy = CHAR_CY;
  // Breathing radius
  int baseR = 28 + ((frame % 8 < 4) ? (frame % 4) : (4 - frame % 4));
  for (int r = baseR; r > 10; r -= 8) {
    // Draw sparse circle (every 4th pixel)
    for (int a = 0; a < 32; a++) {
      const int8_t sinLut8[] = {0, 3, 5, 7, 8, 7, 5, 3, 0, -3, -5, -7, -8, -7, -5, -3};
      const int8_t cosLut8[] = {8, 7, 5, 3, 0, -3, -5, -7, -8, -7, -5, -3, 0, 3, 5, 7};
      int idx = a % 16;
      int16_t px = cx + (r * cosLut8[idx]) / 8;
      int16_t py = cy + (r * sinLut8[idx]) / 8;
      if (inFrame(px, py)) {
        canvas.drawPixel(px, py, dim);
      }
    }
  }
}

// APPROVE: radial action lines (rotating outward from center)
void drawBgRadial(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  uint16_t dim = dimRGB565(color, 2);
  int16_t cx = CHAR_CX;
  int16_t cy = CHAR_CY;
  for (int i = 0; i < 8; i++) {
    int angle = (i * 45 + frame * 15) % 360;
    const int8_t sinLut[] = {0, 71, 100, 71, 0, -71, -100, -71};
    const int8_t cosLut[] = {100, 71, 0, -71, -100, -71, 0, 71};
    int idx = (angle / 45) % 8;
    // Draw line segment from r=25 to r=38
    for (int r = 25; r < 38; r += 2) {
      int16_t px = cx + (r * cosLut[idx]) / 100;
      int16_t py = cy + (r * sinLut[idx]) / 100;
      if (inFrame(px, py)) {
        canvas.drawPixel(px, py, dim);
      }
    }
  }
}

// ERROR: no background (screen shake handled in renderFrame)
void drawBgNone(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  // Screen shake is handled by offset in renderFrame, not bg effect
}

// SLEEPING: fireflies (slow-drifting dots with occasional glow)
void drawBgFireflies(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  uint16_t dim = dimRGB565(color, 1);
  for (int i = 0; i < 5; i++) {
    int16_t baseX = FRAME_X + 10 + ((i * 31 + 5) % (FRAME_W - 20));
    int16_t baseY = FRAME_Y + 10 + ((i * 17 + 3) % (FRAME_H - 20));
    // Slow drift
    const int8_t drift[] = {0, 1, 1, 0, -1, -1};
    int phase = (frame / 2 + i) % 6;
    int16_t x = baseX + drift[phase];
    int16_t y = baseY + drift[(phase + 2) % 6];
    if (inFrame(x, y)) {
      // Occasional bright glow
      if ((frame + i * 3) % 8 < 2) {
        canvas.drawPixel(x, y, color);
        canvas.drawPixel(x + 1, y, dim);
        canvas.drawPixel(x - 1, y, dim);
        canvas.drawPixel(x, y + 1, dim);
        canvas.drawPixel(x, y - 1, dim);
      } else {
        canvas.drawPixel(x, y, dim);
      }
    }
  }
}

// Background effect dispatcher
void drawBackground(M5Canvas& canvas, const char* status, bool sleeping,
                    uint8_t frame, uint16_t accent) {
  if (sleeping) {
    drawBgFireflies(canvas, frame, accent);
    return;
  }
  if (strcmp(status, "READY") == 0)    drawBgStarfield(canvas, frame, accent);
  else if (strcmp(status, "WORKING") == 0)  drawBgWave(canvas, frame, accent);
  else if (strcmp(status, "TOOL") == 0)     drawBgSpeedLines(canvas, frame, accent);
  else if (strcmp(status, "DONE") == 0)     drawBgConfetti(canvas, frame, accent);
  else if (strcmp(status, "INPUT") == 0)    drawBgPulse(canvas, frame, accent);
  else if (strcmp(status, "APPROVE") == 0)  drawBgRadial(canvas, frame, accent);
  else if (strcmp(status, "ERROR") == 0)    drawBgNone(canvas, frame, accent);
}

// ============================================================
// Particle effects (drawn around/near sprite)
// ============================================================

void drawSparkle(M5Canvas& canvas, int16_t x, int16_t y, uint16_t color) {
  canvas.drawPixel(x, y, color);
  canvas.drawPixel(x - 1, y, color);
  canvas.drawPixel(x + 1, y, color);
  canvas.drawPixel(x, y - 1, color);
  canvas.drawPixel(x, y + 1, color);
}

void drawSparkles(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  const int16_t sx[] = { CHAR_X - 6, CHAR_X + SPRITE_W + 4, CHAR_X + 12, CHAR_X + SPRITE_W - 8 };
  const int16_t sy[] = { CHAR_Y + 12, CHAR_Y + 8, CHAR_Y - 4, CHAR_Y + SPRITE_H - 8 };
  for (int i = 0; i < 4; i++) {
    if ((frame + i) % 3 != 0 && inFrame(sx[i], sy[i])) {
      drawSparkle(canvas, sx[i], sy[i], color);
    }
  }
}

void drawThoughtDots(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  int16_t baseY = CHAR_Y - 6;
  int16_t baseX = CHAR_CX - 8;
  for (int i = 0; i < 3; i++) {
    int16_t x = baseX + i * 8;
    int16_t y = baseY - ((frame + i) % 4);
    int r = ((frame + i) % 3 == 0) ? 3 : 2;
    if (inFrame(x, y)) {
      canvas.fillCircle(x, y, r, color);
    }
  }
}

void drawDustPuffs(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  int16_t baseX = CHAR_X - 4;
  int16_t baseY = CHAR_Y + SPRITE_H - 8;
  for (int i = 0; i < 3; i++) {
    int16_t x = baseX - (frame % 4) * 2 - i * 6;
    int16_t y = baseY + ((i + frame) % 3) - 1;
    int r = 3 - i;
    if (r > 0 && inFrame(x, y)) {
      canvas.fillCircle(x, y, r, color);
    }
  }
}

void drawBouncingQuestion(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  int16_t x = CHAR_X + SPRITE_W + 4;
  int bounce[] = {0, -2, -4, -5, -4, -2, 0, 1};
  int16_t y = CHAR_Y + 12 + bounce[frame % 8];
  if (inFrame(x, y)) {
    canvas.setTextColor(color);
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextDatum(middle_center);
    canvas.drawString("?", x, y);
  }
}

void drawPulsingBang(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  int16_t x = CHAR_X + SPRITE_W + 4;
  int16_t y = CHAR_Y + 14;
  if ((frame % 4) < 2 && inFrame(x, y)) {
    canvas.setTextColor(color);
    canvas.setFont(&fonts::FreeSansBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextDatum(middle_center);
    canvas.drawString("!", x, y);
  }
}

void drawCirclingStars(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  int16_t cx = CHAR_CX;
  int16_t cy = CHAR_Y - 2;
  const int radius = 16;
  for (int i = 0; i < 3; i++) {
    int angle = (frame * 30 + i * 120) % 360;
    const int8_t sinLut[] = {0, 71, 100, 71, 0, -71, -100, -71};
    const int8_t cosLut[] = {100, 71, 0, -71, -100, -71, 0, 71};
    int idx = (angle / 45) % 8;
    int16_t sx = cx + (radius * cosLut[idx]) / 100;
    int16_t sy = cy + (radius * sinLut[idx]) / 100;
    if (inFrame(sx, sy)) {
      canvas.drawPixel(sx, sy, color);
      canvas.drawPixel(sx - 1, sy - 1, color);
      canvas.drawPixel(sx + 1, sy - 1, color);
      canvas.drawPixel(sx - 1, sy + 1, color);
      canvas.drawPixel(sx + 1, sy + 1, color);
    }
  }
}

void drawFloatingZZZ(M5Canvas& canvas, uint8_t frame, uint16_t color) {
  canvas.setTextColor(color);
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  canvas.setTextDatum(middle_center);

  int16_t baseX = CHAR_CX + 20;
  for (int i = 0; i < 3; i++) {
    int phase = (frame + i * 3) % 12;
    int16_t x = baseX + i * 4;
    int16_t y = CHAR_Y - 3 - phase * 2;
    if (y > CHAR_Y - 25 && y < CHAR_Y && inFrame(x, y)) {
      canvas.drawString("Z", x, y);
    }
  }
}

// ============================================================
// Word-wrap helper (Font0, 6px per char)
// ============================================================

int drawWrappedText(M5Canvas& cv, const char* text, int16_t x, int16_t y,
                    int maxWidth, int lineHeight, int maxLines, uint16_t color) {
  cv.setTextColor(color);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  cv.setTextDatum(top_left);

  const int charW = 6;
  int maxChars = maxWidth / charW;
  if (maxChars < 1) maxChars = 1;

  int lines = 0;
  int len = strlen(text);
  int pos = 0;

  while (pos < len && lines < maxLines) {
    // How many chars left?
    int remaining = len - pos;
    if (remaining <= maxChars) {
      // Fits on one line
      cv.drawString(text + pos, x, y + lines * lineHeight);
      lines++;
      break;
    }

    // Find last space within maxChars
    int breakAt = maxChars;
    for (int i = maxChars; i > 0; i--) {
      if (text[pos + i] == ' ') {
        breakAt = i;
        break;
      }
    }

    // Draw this line (copy to local buffer to avoid mutating const input)
    char lineBuf[64];
    int segLen = (breakAt < (int)sizeof(lineBuf) - 1) ? breakAt : (int)sizeof(lineBuf) - 1;
    memcpy(lineBuf, text + pos, segLen);
    lineBuf[segLen] = '\0';
    cv.drawString(lineBuf, x, y + lines * lineHeight);

    lines++;
    pos += breakAt;
    // Skip the space
    if (pos < len && text[pos] == ' ') pos++;
  }

  return lines;
}

// ============================================================
// Quest text scroll (y=162-205, shown for APPROVE/INPUT messages)
// ============================================================

// Quest scroll layout
#define QSCROLL_X     8
#define QSCROLL_Y     152
#define QSCROLL_W     119
#define QSCROLL_H     53
#define QSCROLL_LINE_H  10
#define QSCROLL_PAD      4
#define QSCROLL_VISIBLE  ((QSCROLL_H - QSCROLL_PAD * 2) / QSCROLL_LINE_H)  // 4 lines

// Word-wrap into line buffer (returns total line count)
static int wrapLines(const char* text, int maxChars, int* lineStarts, int* lineLens, int maxLines) {
  int len = strlen(text);
  int pos = 0;
  int lines = 0;

  while (pos < len && lines < maxLines) {
    int remaining = len - pos;
    if (remaining <= maxChars) {
      lineStarts[lines] = pos;
      lineLens[lines] = remaining;
      lines++;
      break;
    }
    // Find last space within maxChars
    int breakAt = maxChars;
    for (int i = maxChars; i > 0; i--) {
      if (text[pos + i] == ' ') {
        breakAt = i;
        break;
      }
    }
    lineStarts[lines] = pos;
    lineLens[lines] = breakAt;
    lines++;
    pos += breakAt;
    if (pos < len && text[pos] == ' ') pos++;
  }
  return lines;
}

void drawQuestScroll(M5Canvas& canvas, const char* messageText, uint16_t accent, uint8_t frame) {
  if (messageText[0] == '\0') return;

  uint16_t dim = dimRGB565(accent, 1);
  uint16_t vdim = dimRGB565(accent, 2);

  // Thin single border
  canvas.drawRect(QSCROLL_X, QSCROLL_Y, QSCROLL_W, QSCROLL_H, vdim);

  // Small corner accents (3px)
  canvas.drawFastHLine(QSCROLL_X, QSCROLL_Y, 3, dim);
  canvas.drawFastVLine(QSCROLL_X, QSCROLL_Y, 3, dim);
  canvas.drawFastHLine(QSCROLL_X + QSCROLL_W - 3, QSCROLL_Y, 3, dim);
  canvas.drawFastVLine(QSCROLL_X + QSCROLL_W - 1, QSCROLL_Y, 3, dim);
  canvas.drawFastHLine(QSCROLL_X, QSCROLL_Y + QSCROLL_H - 1, 3, dim);
  canvas.drawFastVLine(QSCROLL_X, QSCROLL_Y + QSCROLL_H - 3, 3, dim);
  canvas.drawFastHLine(QSCROLL_X + QSCROLL_W - 3, QSCROLL_Y + QSCROLL_H - 1, 3, dim);
  canvas.drawFastVLine(QSCROLL_X + QSCROLL_W - 1, QSCROLL_Y + QSCROLL_H - 3, 3, dim);

  // Word-wrap the message
  const int charW = 6;
  int maxChars = (QSCROLL_W - 8) / charW;
  if (maxChars < 1) maxChars = 1;

  int lineStarts[20], lineLens[20];
  int totalLines = wrapLines(messageText, maxChars, lineStarts, lineLens, 20);
  int visible = QSCROLL_VISIBLE;

  // Text drawing area (clipped)
  int16_t textX = QSCROLL_X + QSCROLL_PAD;
  int16_t textY = QSCROLL_Y + QSCROLL_PAD;
  int16_t clipBottom = QSCROLL_Y + QSCROLL_H - QSCROLL_PAD;

  canvas.setTextColor(vdim);
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);
  canvas.setTextDatum(top_left);

  if (totalLines <= visible) {
    // Fits — draw all lines, no scroll
    char lineBuf[32];
    for (int i = 0; i < totalLines; i++) {
      int n = lineLens[i];
      if (n > 31) n = 31;
      memcpy(lineBuf, messageText + lineStarts[i], n);
      lineBuf[n] = '\0';
      canvas.drawString(lineBuf, textX, textY + i * QSCROLL_LINE_H);
    }
  } else {
    // Smooth pixel scroll: text scrolls up continuously, exits view, then loops
    // Total height of all text in pixels
    int totalH = totalLines * QSCROLL_LINE_H;
    int viewH = visible * QSCROLL_LINE_H;
    // Scroll distance: text starts at top, scrolls until fully out, then resets
    // pause (viewH) + scroll through all text (totalH) + scroll out (viewH)
    int scrollRange = viewH + totalH;
    // Speed: 1 pixel per frame (5px/sec at 5 FPS)
    // Add a pause of 15 frames (~3s) at the start before scrolling begins
    int pauseFrames = 15;
    int scrollFrame = (int)frame;
    int cycleLen = pauseFrames + scrollRange;
    int pos = scrollFrame % cycleLen;

    int pixelOffset;
    if (pos < pauseFrames) {
      pixelOffset = 0;  // paused at top
    } else {
      pixelOffset = pos - pauseFrames;
    }

    char lineBuf[32];
    for (int i = 0; i < totalLines; i++) {
      int16_t drawY = textY + i * QSCROLL_LINE_H - pixelOffset;
      // Skip if outside visible area
      if (drawY + QSCROLL_LINE_H < textY || drawY >= clipBottom) continue;

      int n = lineLens[i];
      if (n > 31) n = 31;
      memcpy(lineBuf, messageText + lineStarts[i], n);
      lineBuf[n] = '\0';

      // Only draw if fully within clip bounds
      if (drawY >= textY && drawY + 8 <= clipBottom) {
        canvas.drawString(lineBuf, textX, drawY);
      }
    }

    // Scroll indicator: small down arrow if more text below view
    int bottomLine = (pixelOffset + viewH) / QSCROLL_LINE_H;
    if (bottomLine < totalLines && pixelOffset < totalH) {
      int16_t ax = QSCROLL_X + QSCROLL_W - 8;
      int16_t ay = QSCROLL_Y + QSCROLL_H - 6;
      canvas.drawPixel(ax, ay, dim);
      canvas.drawPixel(ax - 1, ay - 1, dim);
      canvas.drawPixel(ax + 1, ay - 1, dim);
    }
  }
}

// ============================================================
// Boot sequence
// ============================================================

// Starfield data — 30 stars with fixed angle/radius pairs (deterministic)
struct BootStar {
  int8_t dx;   // direction x (-100 to 100, normalized)
  int8_t dy;   // direction y (-100 to 100, normalized)
  uint8_t brightness; // 0-2 brightness tier
};

static const BootStar bootStars[] PROGMEM = {
  { 80,  30, 0}, {-50,  70, 1}, { 20, -90, 0}, {-80, -40, 2}, { 60, -60, 1},
  {-30,  85, 0}, { 90, -10, 2}, {-70, -55, 1}, { 10,  95, 0}, { 75,  65, 2},
  {-95,  15, 0}, { 45, -80, 1}, {-25,  60, 2}, { 85,  45, 0}, {-60, -75, 1},
  { 35,  90, 2}, {-85,  25, 0}, { 55, -45, 1}, {-15, -95, 0}, { 70,  50, 2},
  {-40,  80, 1}, { 95,  -5, 0}, {-55, -65, 2}, { 25,  75, 1}, {-90,  35, 0},
  { 50, -70, 2}, {-65,  50, 1}, { 15, -85, 0}, { 65,  20, 2}, {-45, -80, 1},
};
#define BOOT_STAR_COUNT 30

// Draw 16x16 1-bit icon scaled 2x (32x32)
void drawIcon16Scaled(M5Canvas& canvas, const uint8_t* bitmap, int16_t x, int16_t y,
                      uint16_t color, uint8_t scale = 2) {
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 16; col++) {
      int byteIdx = row * 2 + (col / 8);
      int bitIdx = 7 - (col % 8);
      uint8_t b = pgm_read_byte(&bitmap[byteIdx]);
      if (b & (1 << bitIdx)) {
        canvas.fillRect(x + col * scale, y + row * scale, scale, scale, color);
      }
    }
  }
}

// Draw nebula background — large soft colored clouds to fill the black
void drawBootNebula(M5Canvas& canvas, int16_t cx, int16_t cy, float breathe = 0.5f) {
  // Deep purple base wash across the whole screen
  canvas.fillCircle(cx, cy, 80, dimRGB565(COL_MAGENTA, 5));
  // Large nebula clouds
  int r1 = 40 + (int)(breathe * 8);
  int r2 = 35 + (int)((1.0f - breathe) * 7);
  int r3 = 30 + (int)(breathe * 5);
  canvas.fillCircle(cx - 35, cy + 20, r1, dimRGB565(COL_MAGENTA, 3));
  canvas.fillCircle(cx + 30, cy - 25, r2, dimRGB565(COL_CYAN, 3));
  canvas.fillCircle(cx + 10, cy + 50, r3, dimRGB565(0x4810, 2));  // deep blue
  canvas.fillCircle(cx - 20, cy - 40, r2 - 5, dimRGB565(COL_CYAN, 4));
  canvas.fillCircle(cx + 40, cy + 35, r1 - 10, dimRGB565(COL_MAGENTA, 4));
}

// Draw "Clawy" with per-character colors, big bold font + drop shadow
void drawTitleColored(M5Canvas& canvas, int16_t cx, int16_t y,
                      const uint16_t* palette, int palSize, int offset = 0) {
  const char* title = "Clawy";
  canvas.setFont(&fonts::FreeSansBold18pt7b);
  canvas.setTextSize(1);
  canvas.setTextDatum(middle_left);
  int titleW = canvas.textWidth(title);
  int xCursor = cx - titleW / 2;
  char ch[2] = {0, 0};
  // Drop shadow pass (offset 2px down-right)
  for (int ci = 0; ci < 5; ci++) {
    ch[0] = title[ci];
    canvas.setTextColor(dimRGB565(palette[(ci + offset) % palSize], 3));
    canvas.drawString(ch, xCursor + 2, y + 2);
    xCursor += canvas.textWidth(ch);
  }
  // Foreground pass
  xCursor = cx - titleW / 2;
  for (int ci = 0; ci < 5; ci++) {
    ch[0] = title[ci];
    canvas.setTextColor(palette[(ci + offset) % palSize]);
    canvas.drawString(ch, xCursor, y);
    xCursor += canvas.textWidth(ch);
  }
}

void drawBootScreen(M5Canvas& canvas, unsigned long bootStart,
                    const char* wifiIP = nullptr) {
  unsigned long elapsed = millis() - bootStart;
  canvas.fillSprite(COL_BLACK);

  int16_t cx = SCREEN_W / 2;
  int16_t cy = SCREEN_H / 2 - 20;  // slightly above center

  // ── Phase 1: Starfield zoom (0–700ms) ──
  if (elapsed < 700) {
    float t = (float)elapsed / 700.0f;  // 0→1

    // Nebula glow — fades in during second half
    if (t > 0.3f) {
      float nebT = (t - 0.3f) / 0.7f;  // 0→1
      // Grow the nebula in as t increases
      drawBootNebula(canvas, cx, cy, nebT * 0.5f);
    }

    for (int i = 0; i < BOOT_STAR_COUNT; i++) {
      BootStar s;
      memcpy_P(&s, &bootStars[i], sizeof(BootStar));

      // Stars accelerate outward: radius grows quadratically
      float r = t * t * 1.5f;  // 0→1.5
      int16_t x = cx + (int16_t)(s.dx * r);
      int16_t y = cy + (int16_t)(s.dy * r);

      if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        // Stars get brighter as they move outward
        uint8_t bright;
        if (t < 0.3f) {
          bright = 8;
        } else if (t < 0.6f) {
          bright = 16 + s.brightness * 4;
        } else {
          bright = 24 + s.brightness * 4;
        }
        // Colored stars from palette
        uint16_t col = lerpRGB565(COL_BLACK, starHues[i % 8], bright * 8);

        // Stretch into short trail at high speed
        if (t > 0.4f) {
          int16_t tx = cx + (int16_t)(s.dx * (r - 0.15f));
          int16_t ty = cy + (int16_t)(s.dy * (r - 0.15f));
          canvas.drawLine(tx, ty, x, y, col);
        } else {
          canvas.drawPixel(x, y, col);
          if (s.brightness > 0) {
            canvas.drawPixel(x + 1, y, col);
          }
        }
      }
    }

    canvas.pushSprite(0, 0);
    return;
  }

  // ── Phase 2: Title shimmer (700–1500ms) ──
  // Stars settle, paw drops from top, "Clawy" revealed L→R with sparkle
  if (elapsed < 1500) {
    float t = (float)(elapsed - 700) / 800.0f;  // 0→1

    // Nebula background (fully in by now)
    drawBootNebula(canvas, cx, cy, 0.5f);

    // Settled stars (dim, colored, fading out)
    for (int i = 0; i < BOOT_STAR_COUNT; i++) {
      BootStar s;
      memcpy_P(&s, &bootStars[i], sizeof(BootStar));
      int16_t x = cx + (int16_t)(s.dx * 1.5f);
      int16_t y = cy + (int16_t)(s.dy * 1.5f);
      if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        uint8_t bright = (uint8_t)((1.0f - t) * 96);
        uint16_t col = lerpRGB565(COL_BLACK, starHues[i % 8], bright);
        canvas.drawPixel(x, y, col);
      }
    }

    // Paw icon drops from top → rests at y=45 (2x scaled = 32px)
    int16_t pawTargetY = 45;
    int16_t pawY;
    if (t < 0.4f) {
      float dropT = t / 0.4f;
      float ease = dropT * dropT;
      pawY = -32 + (int16_t)((pawTargetY + 32) * ease);
    } else {
      pawY = pawTargetY;
    }
    // White during drop, lerps to pink after landing
    uint16_t pawColor;
    if (t < 0.4f) {
      pawColor = COL_WHITE;
    } else {
      uint8_t pinkT = (uint8_t)((t - 0.4f) / 0.6f * 255);
      pawColor = lerpRGB565(COL_WHITE, COL_PINK, pinkT);
    }
    drawIcon16Scaled(canvas, icon_paw_16x16, cx - 16, pawY, pawColor);

    // "Clawy" revealed left→right with per-char gradient + sparkle
    const char* title = "Clawy";
    canvas.setFont(&fonts::FreeSansBold18pt7b);
    canvas.setTextSize(1);

    int titleW = canvas.textWidth(title);
    int titleX = cx - titleW / 2;
    int titleY = 100;

    if (t > 0.3f) {
      float revealT = (t - 0.3f) / 0.6f;  // 0→1 over reveal period
      if (revealT > 1.0f) revealT = 1.0f;
      int revealW = (int)(revealT * titleW);

      // Draw per-character colored title, then mask unrevealed portion
      drawTitleColored(canvas, cx, titleY, titleGrad, 5);
      if (revealW < titleW) {
        // Mask rect needs to cover the full glyph height (18pt ~ 30px)
        canvas.fillRect(titleX + revealW, titleY - 18, titleW - revealW + 4, 40, COL_BLACK);

        // Sparkle at sweep edge — determine which character is being revealed
        int xAcc = 0;
        int sparkCharIdx = 0;
        char ch[2] = {0, 0};
        for (int ci = 0; ci < 5; ci++) {
          ch[0] = title[ci];
          int cw = canvas.textWidth(ch);
          if (xAcc + cw > revealW) { sparkCharIdx = ci; break; }
          xAcc += cw;
        }
        uint16_t sparkColor = titleGrad[sparkCharIdx % 5];

        int16_t sparkX = titleX + revealW;
        int16_t sparkY = titleY;
        drawSparkle(canvas, sparkX, sparkY, sparkColor);
        uint16_t sparkDim = dimRGB565(sparkColor, 1);
        canvas.drawPixel(sparkX - 2, sparkY, sparkDim);
        canvas.drawPixel(sparkX + 2, sparkY, sparkDim);
        canvas.drawPixel(sparkX, sparkY - 2, sparkDim);
        canvas.drawPixel(sparkX, sparkY + 2, sparkDim);
      }
    }

    canvas.pushSprite(0, 0);
    return;
  }

  // ── Phase 3: Tagline fade in (1500–2100ms) ──
  if (elapsed < 2100) {
    float t = (float)(elapsed - 1500) / 600.0f;  // 0→1
    if (t > 1.0f) t = 1.0f;

    // Nebula background
    drawBootNebula(canvas, cx, cy, 0.5f);

    // Pink paw (2x) + gradient title
    drawIcon16Scaled(canvas, icon_paw_16x16, cx - 16, 45, COL_PINK);
    drawTitleColored(canvas, cx, 100, titleGrad, 5);

    // Tagline fades in with cyan tint
    uint8_t g = (uint8_t)(t * 16);
    uint16_t tagColor = ((g / 3) << 11) | (g << 5) | (g / 2);
    canvas.setTextColor(tagColor);
    canvas.setFont(&fonts::Font0);
    canvas.setTextDatum(middle_center);
    canvas.drawString("a claude code", cx, 128);
    canvas.drawString("companion", cx, 140);

    canvas.pushSprite(0, 0);
    return;
  }

  // ── Phase 4: Title screen idle (2100ms+, loops until Button A) ──
  {
    unsigned long idle = elapsed - 2100;

    // Nebula wash — breathing clouds fill the background
    {
      float breathe = 0.5f + 0.5f * sinf((float)idle * 0.002f);
      drawBootNebula(canvas, cx, cy, breathe);
    }

    // Twinkling colored stars
    for (int i = 0; i < BOOT_STAR_COUNT; i++) {
      BootStar s;
      memcpy_P(&s, &bootStars[i], sizeof(BootStar));
      int16_t x = cx + (int16_t)(s.dx * 1.5f);
      int16_t y = cy + (int16_t)(s.dy * 1.5f);
      if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        float phase = (float)(idle + i * 200) * 0.001f;
        float flicker = 0.5f + 0.5f * sinf(phase * (1.0f + s.brightness * 0.5f));
        uint8_t bright = (uint8_t)(flicker * (32 + s.brightness * 32));
        uint16_t col = lerpRGB565(COL_BLACK, starHues[i % 8], bright);
        canvas.drawPixel(x, y, col);
      }
    }

    // Paw icon (2x) with gentle bob — slow warm color cycle (pink → orange → yellow)
    float bob = sinf((float)idle * 0.002f) * 2.0f;
    float pawPhase = sinf((float)idle * 0.0015f);  // -1→1
    uint16_t pawCol;
    if (pawPhase >= 0) {
      pawCol = lerpRGB565(COL_PINK, COL_ORANGE, (uint8_t)(pawPhase * 255));
    } else {
      pawCol = lerpRGB565(COL_PINK, COL_YELLOW, (uint8_t)(-pawPhase * 255));
    }
    drawIcon16Scaled(canvas, icon_paw_16x16, cx - 16, 45 + (int16_t)bob, pawCol);

    // Rotating title gradient — offset shifts every 2s (SNES color wave)
    int titleOffset = (int)(idle / 2000) % 8;
    drawTitleColored(canvas, cx, 100, titlePalette, 8, titleOffset);

    // Cyan-gray tagline
    uint16_t tagCol = dimRGB565(COL_CYAN, 2);
    canvas.setTextColor(tagCol);
    canvas.setFont(&fonts::Font0);
    canvas.setTextDatum(middle_center);
    canvas.drawString("a claude code", cx, 128);
    canvas.drawString("companion", cx, 140);

    // "- PRESS START -" blinks 500ms on/off, "on" color rotates white → cyan → yellow
    if ((idle % 1000) < 500) {
      float startPhase = sinf((float)idle * 0.001f);
      uint16_t startCol;
      if (startPhase >= 0) {
        startCol = lerpRGB565(COL_WHITE, COL_CYAN, (uint8_t)(startPhase * 200));
      } else {
        startCol = lerpRGB565(COL_WHITE, COL_YELLOW, (uint8_t)(-startPhase * 200));
      }
      canvas.setTextColor(startCol);
      canvas.setFont(&fonts::Font0);
      canvas.setTextDatum(middle_center);
      canvas.drawString("- PRESS START -", cx, 170);
    }

    // Show "CONNECTED" when WiFi is up (small, dim, below PRESS START)
    if (wifiIP && wifiIP[0]) {
      canvas.setTextColor(dimRGB565(COL_GREEN, 1));
      canvas.setFont(&fonts::Font0);
      canvas.setTextDatum(middle_center);
      canvas.drawString("CONNECTED", cx, 215);
    }

    canvas.pushSprite(0, 0);
    return;
  }
}

// ============================================================
// Stats screen (JRPG-styled)
// ============================================================

struct Stats {
  uint16_t promptCount;
  uint16_t toolCallCount;
  uint16_t errorCount;
  uint16_t doneCount;
  unsigned long totalResponseMs;
  uint16_t responseCount;  // for averaging
};

void drawStatsScreen(M5Canvas& canvas, const Stats& stats, unsigned long sessionStart,
                     uint16_t accent) {
  uint16_t dim = dimRGB565(accent, 1);

  // Stats window fills portrait + text window area
  int sx = FRAME_X;
  int sy = FRAME_Y;
  int sw = FRAME_W;
  int sh = TBOX_Y + TBOX_H - FRAME_Y;

  // Background
  canvas.fillRect(sx + 1, sy + 1, sw - 2, sh - 2, COL_BLACK);

  // Double border
  canvas.drawRect(sx, sy, sw, sh, dim);
  canvas.drawRect(sx + 2, sy + 2, sw - 4, sh - 4, dimRGB565(accent, 2));

  // Corner brackets
  int bLen = 8;
  canvas.drawFastHLine(sx, sy, bLen, accent);
  canvas.drawFastVLine(sx, sy, bLen, accent);
  canvas.drawFastHLine(sx + sw - bLen, sy, bLen, accent);
  canvas.drawFastVLine(sx + sw - 1, sy, bLen, accent);
  canvas.drawFastHLine(sx, sy + sh - 1, bLen, accent);
  canvas.drawFastVLine(sx, sy + sh - bLen, bLen, accent);
  canvas.drawFastHLine(sx + sw - bLen, sy + sh - 1, bLen, accent);
  canvas.drawFastVLine(sx + sw - 1, sy + sh - bLen, bLen, accent);

  // Title
  canvas.setTextColor(accent);
  canvas.setFont(&fonts::FreeSansBold9pt7b);
  canvas.setTextSize(1);
  canvas.setTextDatum(middle_center);
  canvas.drawString("~ Stats ~", SCREEN_W / 2, sy + 16);

  // Divider line under title
  canvas.drawFastHLine(sx + 8, sy + 26, sw - 16, dimRGB565(accent, 2));

  // Stat rows
  canvas.setFont(&fonts::Font0);
  canvas.setTextSize(1);

  const char* labels[5] = {"Prompts", "Tools", "Errors", "Session", "Avg Resp"};
  char values[5][12];

  snprintf(values[0], sizeof(values[0]), "%u", stats.promptCount);
  snprintf(values[1], sizeof(values[1]), "%u", stats.toolCallCount);
  snprintf(values[2], sizeof(values[2]), "%u", stats.errorCount);

  // Session time
  unsigned long sec = (millis() - sessionStart) / 1000;
  if (sec < 60) {
    snprintf(values[3], sizeof(values[3]), "%lus", sec);
  } else if (sec < 3600) {
    snprintf(values[3], sizeof(values[3]), "%lu:%02lu", sec / 60, sec % 60);
  } else {
    snprintf(values[3], sizeof(values[3]), "%luh%02lu", sec / 3600, (sec % 3600) / 60);
  }

  // Avg response
  if (stats.responseCount > 0) {
    unsigned long avg = stats.totalResponseMs / stats.responseCount / 1000;
    snprintf(values[4], sizeof(values[4]), "%lus", avg);
  } else {
    strcpy(values[4], "--");
  }

  int rowY = sy + 34;
  for (int i = 0; i < 5; i++) {
    canvas.setTextColor(dim);
    canvas.setTextDatum(middle_left);
    canvas.drawString(labels[i], sx + 10, rowY);
    canvas.setTextColor(accent);
    canvas.setTextDatum(middle_right);
    canvas.drawString(values[i], sx + sw - 10, rowY);

    // Dotted line between label and value
    for (int dx = 52; dx < sw - 36; dx += 3) {
      canvas.drawPixel(sx + dx, rowY, dimRGB565(accent, 3));
    }

    rowY += 18;
  }
}
