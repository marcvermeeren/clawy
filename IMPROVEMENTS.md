# Plan: Clawy — Codebase Improvements

## Context

Clawy is a working open-source release (M5StickC Plus 2 companion for Claude Code). The prototype-to-release conversion is done. This plan covers improvements to firmware, scripts, and the web flasher — focused on what actually matters for a local-network hobby device with a small audience.

Findings come from a full codebase audit across firmware, hook scripts, installer, and web flasher.

---

## Tier 1: Actually Dangerous (fix before shipping)

### 1A. Approval spoofing — anyone on LAN can approve/deny actions
**File:** `firmware/clawy/wifi_transport.h`
- TCP port 7801 accepts `BUTTON:APPROVE` / `BUTTON:DENY` from any client on the network
- This is the most serious issue: a rogue process on the network could auto-approve dangerous actions Claude Code wants to run
- **Fix:** Add a shared secret (random token generated at boot, displayed on device screen, entered in install.sh config). Approval messages must include the token: `BUTTON:APPROVE:<token>`. Scripts send token from `~/.clawy/config`.

### 1B. WiFi provisioning protocol breaks on colons in SSID/password
**File:** `firmware/clawy/wifi_provision.h` (line 90) + `web/index.html` (line 453)
- Protocol is `WIFI:<ssid>:<password>\n` — parsed by first colon after `WIFI:`
- SSID or password containing `:` breaks parsing (e.g. `WIFI:my:network:pass123`)
- **Fix firmware:** Parse as `WIFI:<ssid>:<password>` where only the FIRST colon after pos 5 is the separator (password gets everything after). Already works for password (line 100 uses `sep + 1` to end), but SSID with colon would truncate.
- **Fix web:** Validate SSID doesn't contain colons, or URL-encode the fields.

### 1C. Unsafe const-cast string mutation in display word-wrap
**File:** `firmware/clawy/display.h` (lines 636-640)
- Casts `const char*` to `char*` and modifies it to insert null terminators for word wrapping
- If text is in flash/PROGMEM, this corrupts memory
- **Fix:** Copy the line segment to a local buffer before drawing.

### 1D. CDN script loaded without integrity check
**File:** `web/index.html` (line 414)
- `esp-web-tools` loaded from unpkg with no version pin or SRI hash
- If CDN is compromised, arbitrary JS runs on the flash page
- **Fix:** Pin version + add `integrity` attribute, or self-host the script.

---

## Tier 2: Real bugs users will hit

### 2A. WiFi reconnection missing — device goes dead after brief dropout
**File:** `firmware/clawy/wifi_transport.h` (lines 91-105)
- `wifiCheck()` detects disconnect but never reconnects
- Once WiFi drops (router restart, interference), device is dead until manually rebooted
- **Fix:** Add `WiFi.reconnect()` when `!up && _wifiUp`, with backoff (try every 5s, give up after 60s, then try every 30s).

### 2B. `nc` flags not portable to Linux
**File:** `hooks/send-status.sh` (line 50)
- Uses `nc -G 1 -w 1` — the `-G` flag (connect timeout) is BSD/macOS netcat only
- Linux `nc` (GNU netcat or ncat) uses different flags or doesn't support `-G`
- **Fix:** Detect OS and use appropriate flags, or replace `nc` with Python socket one-liner (python3 is already a dependency).

### 2C. Missing dependency checks in scripts
**Files:** `install.sh`, `hooks/send-status.sh`, `hooks/permission-listener.sh`
- If `python3` is missing, install.sh crashes with a cryptic error
- If `nc` is missing, status updates silently fail
- **Fix:** Add `command -v python3 >/dev/null || { echo "Error: python3 required"; exit 1; }` at top of scripts that need it.

### 2D. Serial reader/writer locks not properly released on error
**File:** `web/index.html` (lines 450-498)
- If `writer.write()` throws after `getWriter()`, the lock isn't released
- If `reader.read()` throws, `releaseLock()` at line 492 may not execute
- Device port becomes unusable until page refresh
- **Fix:** Use try/finally blocks around both writer and reader sections.

### 2E. No serial buffer timeout on device
**File:** `firmware/clawy/clawy.ino` (lines 623-641)
- If a sender writes bytes without a newline, buffer fills and sits there forever
- Next valid command gets prepended with garbage
- **Fix:** Add timeout — if no newline received within 5s, discard buffer.

---

## Tier 3: Polish & robustness

### 3A. Install backup & validation
**File:** `install.sh`
- Back up `settings.json` before modifying: `cp settings.json settings.json.bak`
- Verify hooks were actually written after Python runs
- Verify copied hook files exist and are executable

### 3B. Cache file permissions
**File:** `hooks/send-status.sh` (line 15)
- `/tmp/clawy-ip-$USER` is world-readable by default
- **Fix:** `touch "$CACHE_FILE" && chmod 600 "$CACHE_FILE"` on creation.

### 3C. Shorter cache TTL + invalidation
**Files:** `hooks/send-status.sh`, `hooks/permission-listener.sh`
- 1-hour TTL means device IP changes aren't picked up after reboot
- **Fix:** Reduce to 5 minutes. Add cache bust: if `nc` fails, delete cache and re-resolve on next call.

### 3D. SSID/password length validation in web form
**File:** `web/index.html`
- WiFi spec: SSID max 32 chars, password max 63 chars
- No validation before sending to device
- **Fix:** Add `maxlength` attributes and JS validation.

### 3E. Color support detection in install.sh
**File:** `install.sh`
- ANSI colors render as garbage if piped or on dumb terminals
- **Fix:** Check `[ -t 1 ] && [ "$(tput colors 2>/dev/null)" -ge 8 ]` — if false, set all color vars to empty string.

### 3F. Copy button error handling
**File:** `web/index.html` (lines 503-514)
- `navigator.clipboard.writeText()` promise rejection not caught
- **Fix:** Add `.catch()` with fallback (select text, show "Press Ctrl+C").

---

## Tier 4: New features (pick and choose)

### 4A. Diagnostic script (`diagnose.sh`)
- Check python3, nc versions
- Test mDNS resolution of `clawy.local`
- Attempt TCP connection to device ports 7800/7801
- Verify hooks in `settings.json`
- Print summary: "Everything OK" or list failures

### 4B. Update script (`update.sh`)
- `git pull` + re-run install.sh
- Compare installed hook versions with repo versions

### 4C. Config file (`~/.clawy/config`)
- Source from hook scripts
- User-configurable: hostname, ports, cache TTL, debug mode
- Defaults to `clawy.local`, 7800/7801, 300s, off

### 4D. OTA firmware updates
- Add ElegantOTA library to firmware
- Device exposes `http://clawy.local/update` when connected
- Users can upload new .bin from browser without USB

### 4E. Device settings via button menu
- Long-press Button A to enter settings
- Options: display brightness, WiFi reset, show IP/hostname, show firmware version

---

## Files to modify

| Tier | File | Change |
|------|------|--------|
| 1A | `firmware/clawy/wifi_transport.h` | Add token auth for approval port |
| 1A | `hooks/permission-listener.sh` | Send token with approval |
| 1A | `install.sh` | Generate + store token in `~/.clawy/config` |
| 1B | `firmware/clawy/wifi_provision.h` | Fix colon parsing |
| 1B | `web/index.html` | Validate SSID input |
| 1C | `firmware/clawy/display.h` | Fix word-wrap buffer mutation |
| 1D | `web/index.html` | Pin esp-web-tools version + SRI |
| 2A | `firmware/clawy/wifi_transport.h` | Add reconnection logic |
| 2B | `hooks/send-status.sh` | Replace nc with python socket |
| 2C | `install.sh`, hook scripts | Add dependency checks |
| 2D | `web/index.html` | Fix try/finally for serial locks |
| 2E | `firmware/clawy/clawy.ino` | Add serial buffer timeout |
| 3A-F | Various | See individual items |

---

## Verification

1. Compile firmware: `arduino-cli compile -b m5stack:esp32:m5stack_stickc_plus2 firmware/clawy/`
2. Flash and test WiFi provisioning with SSID containing special chars
3. Test approval flow — verify unauthenticated approval is rejected
4. Kill WiFi briefly (toggle router) — verify device reconnects
5. Run `./install.sh` on Linux — verify nc replacement works
6. Run install.sh with missing python3 — verify clean error message
7. Open web flasher, test WiFi form error paths (disconnect device mid-setup)
8. Pipe install.sh output: `./install.sh | cat` — verify no ANSI garbage
