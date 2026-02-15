# Clawy — JRPG Companion for Claude Code Sessions

## Board

| Key       | Value |
|-----------|-------|
| Board     | M5StickC Plus 2 |
| MCU       | ESP32-PICO-V3-02 (dual-core, WiFi/BT) |
| FQBN      | `m5stack:esp32:m5stack_stickc_plus2` |
| Display   | 1.14" TFT 135x240, ST7789V2, SPI, color |
| Buttons   | A (GPIO37), B (GPIO39) |
| USB Port  | `/dev/cu.usbserial-*` or `/dev/cu.wchusbserial-*` (CH9102F UART bridge) |
| Libraries | M5Unified + M5GFX |

## Commands

```bash
# Compile
arduino-cli compile -b m5stack:esp32:m5stack_stickc_plus2 firmware/clawy/

# Upload (find port: ls /dev/cu.usbserial-* /dev/cu.wchusbserial-*)
arduino-cli upload -b m5stack:esp32:m5stack_stickc_plus2 -p /dev/cu.usbserial-* firmware/clawy/

# Compile + Upload
arduino-cli compile -b m5stack:esp32:m5stack_stickc_plus2 -u -p /dev/cu.usbserial-* firmware/clawy/

# Build for web flasher (outputs .bin to web/)
arduino-cli compile -b m5stack:esp32:m5stack_stickc_plus2 --output-dir web/ firmware/clawy/
```

## WiFi Provisioning

Device supports two WiFi credential sources:
1. **NVS (Improv WiFi)** — configured via browser during flash or re-provisioning
2. **secrets.h (compile-time)** — fallback for developers

Priority: NVS first, secrets.h second. If neither exists, device enters Improv provisioning mode.

Hold both buttons (A+B) during boot to clear NVS WiFi creds and re-enter provisioning.

## Network

| Key | Value |
|-----|-------|
| mDNS hostname | `clawy.local` (configurable via NVS) |
| Command port | TCP 7800 (fire-and-forget) |
| Approval port | TCP 7801 (bidirectional) |

## File Structure

| File | Purpose |
|------|---------|
| `firmware/clawy/clawy.ino` | Main sketch: setup, loop, state machine, protocol |
| `firmware/clawy/display.h` | All rendering: HUD, portrait frame, boot sequence, effects |
| `firmware/clawy/wifi_transport.h` | WiFi/TCP/mDNS: two-port server |
| `firmware/clawy/wifi_provision.h` | Improv WiFi serial + NVS credential storage |
| `firmware/clawy/sprites.h` | 64x64 sprite bitmaps + 16x16 paw icon |
| `firmware/clawy/secrets_example.h` | Credentials template (committed) |
| `hooks/send-status.sh` | Fire-and-forget WiFi status sender |
| `hooks/send-tool-status.sh` | PreToolUse hook: tool name mapper |
| `hooks/permission-listener.sh` | PermissionRequest hook: bidirectional approval |
| `install.sh` | Sets up hooks in ~/.claude/settings.json |
| `uninstall.sh` | Removes hooks and ~/.clawy/ |

## Debugging

**Safe to run from Claude:**
- `arduino-cli compile -b m5stack:esp32:m5stack_stickc_plus2 firmware/clawy/`
- `CLAWY=1 hooks/send-status.sh DONE` — test WiFi status update

**User's terminal only (will hang Claude):**
- `arduino-cli monitor -p /dev/cu.usbserial-* -c baudrate=115200`
