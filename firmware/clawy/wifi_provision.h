#pragma once
#include <WiFi.h>
#include <Preferences.h>

// ============================================================
// WiFi Provisioning — text-based serial protocol
//
// Browser sends: WIFI:<ssid>:<password>\n
// Device replies: WIFI:OK:<ip>\n  or  WIFI:FAIL\n
//
// Credentials stored in NVS for next boot.
// Hold A+B at boot to clear saved creds.
// ============================================================

#define NVS_NAMESPACE "clawy"
#define NVS_KEY_SSID  "ssid"
#define NVS_KEY_PASS  "pass"
#define NVS_KEY_HOST  "hostname"

static Preferences _prefs;
static bool _provisioningMode = false;

static char _provBuf[256];
static uint8_t _provPos = 0;

// ── NVS helpers ─────────────────────────────────────────────

bool nvsLoadWiFiCreds(char* ssid, size_t ssidLen, char* pass, size_t passLen) {
  _prefs.begin(NVS_NAMESPACE, true);
  String s = _prefs.getString(NVS_KEY_SSID, "");
  String p = _prefs.getString(NVS_KEY_PASS, "");
  _prefs.end();
  if (s.length() == 0) return false;
  s.toCharArray(ssid, ssidLen);
  p.toCharArray(pass, passLen);
  return true;
}

void nvsSaveWiFiCreds(const char* ssid, const char* pass) {
  _prefs.begin(NVS_NAMESPACE, false);
  _prefs.putString(NVS_KEY_SSID, ssid);
  _prefs.putString(NVS_KEY_PASS, pass);
  _prefs.end();
}

void nvsClearWiFiCreds() {
  _prefs.begin(NVS_NAMESPACE, false);
  _prefs.remove(NVS_KEY_SSID);
  _prefs.remove(NVS_KEY_PASS);
  _prefs.end();
}

const char* nvsGetHostname() {
  static char hostname[64] = "clawy";
  _prefs.begin(NVS_NAMESPACE, true);
  String h = _prefs.getString(NVS_KEY_HOST, "clawy");
  _prefs.end();
  h.toCharArray(hostname, sizeof(hostname));
  return hostname;
}

// ── WiFi connect attempt ────────────────────────────────────

bool provisionTryConnect(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    nvsSaveWiFiCreds(ssid, pass);
    return true;
  }

  WiFi.disconnect();
  return false;
}

// ── Serial provisioning line handler ────────────────────────

void provisionProcessLine(const char* line) {
  // Expected: WIFI:<ssid>:<password>
  if (strncmp(line, "WIFI:", 5) != 0) return;

  const char* ssidStart = line + 5;
  const char* sep = strchr(ssidStart, ':');
  if (!sep) return;

  // Parse SSID and password
  char ssid[33] = {0};
  char pass[65] = {0};
  int ssidLen = sep - ssidStart;
  if (ssidLen > 32) ssidLen = 32;
  memcpy(ssid, ssidStart, ssidLen);

  const char* passStart = sep + 1;
  strncpy(pass, passStart, 64);

  Serial.println("WIFI:CONNECTING");

  if (provisionTryConnect(ssid, pass)) {
    char ipStr[16];
    WiFi.localIP().toString().toCharArray(ipStr, sizeof(ipStr));
    Serial.printf("WIFI:OK:%s\n", ipStr);
    _provisioningMode = false;
  } else {
    Serial.println("WIFI:FAIL");
  }
}

// ── Serial loop (call from main loop when provisioning) ─────

void provisionLoop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (_provPos > 0) {
        _provBuf[_provPos] = '\0';
        provisionProcessLine(_provBuf);
        _provPos = 0;
      }
    } else if (_provPos < sizeof(_provBuf) - 1) {
      _provBuf[_provPos++] = c;
    }
  }
}

// ── Public API ──────────────────────────────────────────────

bool provisioningIsActive() { return _provisioningMode; }

void provisioningEnter() {
  _provisioningMode = true;
  _provPos = 0;
  Serial.println("WIFI:WAITING");
}
