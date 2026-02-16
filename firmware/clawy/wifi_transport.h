#pragma once
#include <WiFi.h>
#include <ESPmDNS.h>

// secrets.h is optional — WiFi creds can come from NVS (Improv provisioning)
#if __has_include("secrets.h")
#include "secrets.h"
#endif

// Two-port TCP server:
//   7800 = Command (fire-and-forget STATUS/MESSAGE inbound, max 2 clients)
//   7801 = Approval (bidirectional APPROVE flow, max 1 client)
#define CMD_PORT       7800
#define APPROVAL_PORT  7801
#define CMD_MAX        2
#define TCP_BUF        256
#define CMD_STALE_MS   10000  // drop command clients idle >10s

// Forward declarations (defined in .ino)
extern void processLine(const char* line);
extern void onApprovalDisconnect();

static WiFiServer cmdServer(CMD_PORT);
static WiFiServer approvalServer(APPROVAL_PORT);

// Command clients (short-lived, fire-and-forget)
static WiFiClient cmdClients[CMD_MAX];
static char cmdBuf[CMD_MAX][TCP_BUF];
static uint8_t cmdPos[CMD_MAX] = {0, 0};
static unsigned long cmdLastActive[CMD_MAX] = {0, 0};

// Approval client (long-lived, one at a time)
static WiFiClient approvalClient;
static char aprBuf[TCP_BUF];
static uint8_t aprPos = 0;

static bool _wifiUp = false;
static unsigned long _wifiReconnectAt = 0;
#define WIFI_RECONNECT_INTERVAL 5000  // retry every 5s when disconnected

// Approval pending state — keeps client alive even after half-close
static bool _approvalPending = false;
static unsigned long _approvalTimestamp = 0;
#define APPROVAL_TIMEOUT_MS 65000

void wifiSetApprovalPending(bool pending) {
  _approvalPending = pending;
  _approvalTimestamp = pending ? millis() : 0;
}

void wifiClearApproval() {
  if (!_approvalPending) return;
  _approvalPending = false;
  _approvalTimestamp = 0;
  if (approvalClient) {
    approvalClient.stop();
    aprPos = 0;
  }
}

// ── Helpers ──────────────────────────────────────────────────

static void tcpReadLines(WiFiClient& c, char* buf, uint8_t& pos) {
  while (c.available()) {
    char ch = c.read();
    if (ch == '\n' || ch == '\r') {
      if (pos > 0) {
        buf[pos] = '\0';
        processLine(buf);
        pos = 0;
      }
    } else if (pos < TCP_BUF - 1) {
      buf[pos++] = ch;
    }
  }
}

// ── Public API ───────────────────────────────────────────────

// Connect with compile-time credentials (secrets.h fallback)
void wifiBegin() {
#if defined(WIFI_SSID) && defined(WIFI_PASS)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#endif
}

// Connect with runtime credentials (NVS / Improv provisioning)
void wifiBeginWithCreds(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
}

bool wifiIsConnected() { return _wifiUp; }

const char* wifiGetIP() {
  static char ip[16] = "";
  if (_wifiUp) {
    WiFi.localIP().toString().toCharArray(ip, sizeof(ip));
  } else {
    ip[0] = '\0';
  }
  return ip;
}

void wifiCheck() {
  bool up = (WiFi.status() == WL_CONNECTED);
  if (up && !_wifiUp) {
    _wifiUp = true;
    const char* hostname = nvsGetHostname();
    MDNS.begin(hostname);
    cmdServer.begin();
    approvalServer.begin();
    Serial.printf("WiFi: %s (%s.local)\n",
                  WiFi.localIP().toString().c_str(), hostname);
  } else if (!up && _wifiUp) {
    _wifiUp = false;
    _wifiReconnectAt = millis() + WIFI_RECONNECT_INTERVAL;
    Serial.println("WiFi: disconnected, will reconnect");
  } else if (!up && !_wifiUp) {
    // Periodically try to reconnect
    if (millis() >= _wifiReconnectAt) {
      WiFi.reconnect();
      _wifiReconnectAt = millis() + WIFI_RECONNECT_INTERVAL;
    }
  }
}

void wifiPoll() {
  if (!_wifiUp) return;

  // --- Command port (fire-and-forget) ---

  // Clean up stale/disconnected clients first to free slots
  for (int i = 0; i < CMD_MAX; i++) {
    if (!cmdClients[i]) continue;
    if (!cmdClients[i].connected() || (millis() - cmdLastActive[i] > CMD_STALE_MS)) {
      if (cmdPos[i] > 0) {
        cmdBuf[i][cmdPos[i]] = '\0';
        processLine(cmdBuf[i]);
        cmdPos[i] = 0;
      }
      cmdClients[i].stop();
    }
  }

  WiFiClient nc = cmdServer.available();
  if (nc) {
    nc.setNoDelay(true);
    bool stored = false;
    for (int i = 0; i < CMD_MAX; i++) {
      if (!cmdClients[i] || !cmdClients[i].connected()) {
        cmdClients[i] = nc;
        cmdPos[i] = 0;
        cmdLastActive[i] = millis();
        stored = true;
        break;
      }
    }
    if (!stored) nc.stop();
  }

  for (int i = 0; i < CMD_MAX; i++) {
    if (!cmdClients[i] || !cmdClients[i].connected()) continue;
    if (cmdClients[i].available()) {
      tcpReadLines(cmdClients[i], cmdBuf[i], cmdPos[i]);
      cmdLastActive[i] = millis();
    }
  }

  // --- Approval port (bidirectional, max 1 client) ---
  WiFiClient na = approvalServer.available();
  if (na) {
    na.setNoDelay(true);
    // Reject new connections while approval is pending
    if (_approvalPending && approvalClient) {
      na.stop();
    } else if (approvalClient && approvalClient.connected()) {
      na.stop();  // reject — one at a time
    } else {
      approvalClient = na;
      aprPos = 0;
    }
  }

  if (approvalClient) {
    // Read data if available (works even on half-closed sockets)
    if (approvalClient.available()) {
      tcpReadLines(approvalClient, aprBuf, aprPos);
    }

    // Keep alive while approval pending, even if connected() is false
    if (_approvalPending) {
      // Timeout: clear stale pending state after 65s
      if (millis() - _approvalTimestamp > APPROVAL_TIMEOUT_MS) {
        _approvalPending = false;
        approvalClient.stop();
        aprPos = 0;
      } else if (!approvalClient.connected() && !approvalClient.available()) {
        // Hook was killed (terminal approval) — TCP FIN received, no data left
        _approvalPending = false;
        _approvalTimestamp = 0;
        approvalClient.stop();
        aprPos = 0;
        onApprovalDisconnect();
      }
    } else if (!approvalClient.connected()) {
      // Not pending — clean up disconnected client
      if (aprPos > 0) {
        aprBuf[aprPos] = '\0';
        processLine(aprBuf);
        aprPos = 0;
      }
      approvalClient.stop();
    }
  }
}

// Send button response to approval client
void wifiSendApproval(const char* msg) {
  if (approvalClient) {
    approvalClient.println(msg);
    approvalClient.flush();
    _approvalPending = false;
    approvalClient.stop();
  }
}
