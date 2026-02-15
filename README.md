# Clawy

<p align="center">
  <img src="assets/exports/gif/idle.gif" alt="Clawy" width="128" style="image-rendering: pixelated;">
</p>

Clawy is built as a cute pixel companion for [Claude Code](https://docs.anthropic.com/en/docs/claude-code) sessions, a little guy that fits in your pocket that tells you what's going on, lets you approve or deny without touching your keyboard, and turns your whole session into a cozy co-op game.

> This started as a prototype I built for myself. It's rough around the edges but it works, and I'm actively improving it. Contributions and feedback welcome.

## Features

◆ 8 animated states to see what Claude is doing at a glance

◆ Approve or deny straight from the device, no need to be at your computer

★ Powered by Claude Code hooks native integration

♥ Runs on an off-the-shelf M5StickC Plus 2

◆ Works from any project, Clawy follows your sessions

★ JRPG aesthetic portrait frames, dialog boxes, scrolling quest text, particle effects and boot sequence

♥ Zero config after WiFi setup, mDNS discovery, no IPs to manage

◆ Session stats — prompts, tool calls, errors, and average response time

★ Clawy curls up to sleep after 30 seconds of inactivity

♥ No cloud, no server, just plain TCP on your local network, nothing leaves your machine

## What You Need

- **M5StickC Plus 2** (~$20) — [M5Stack store](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit)
- **Claude Code** installed and working
- **USB cable** (USB-C, data-capable)

## Quick Start

### Path A: Flash from Browser (Recommended)

1. Plug in the M5StickC Plus 2 via USB
2. Visit the [web flasher](https://clawy.lol/flash) in Chrome or Edge
3. Click **Install Clawy** and select the serial port
4. Enter your WiFi credentials on the flash page (sent directly to the device over USB — never leaves your computer)
5. Reboot the device after flashing (press and hold the power button, then turn it back on)
6. Install the hooks:
   ```bash
   git clone https://github.com/marcvermeeren/clawy.git
   cd clawy
   ./install.sh
   ```
7. Start a session:
   ```bash
   clawy
   # or: CLAWY=1 claude
   ```

### Path B: Build from Source

1. Clone the repo:
   ```bash
   git clone https://github.com/marcvermeeren/clawy.git
   cd clawy
   ```
2. (Optional) Set compile-time WiFi credentials:
   ```bash
   cp firmware/clawy/secrets_example.h firmware/clawy/secrets.h
   # Edit secrets.h with your WiFi SSID and password
   ```
3. Compile and upload:
   ```bash
   arduino-cli compile -b m5stack:esp32:m5stack_stickc_plus2 firmware/clawy/
   arduino-cli upload -b m5stack:esp32:m5stack_stickc_plus2 -p /dev/cu.usbserial-* firmware/clawy/
   ```
4. Install hooks:
   ```bash
   ./install.sh
   ```
5. Start a session:
   ```bash
   clawy
   ```

## How It Works

Clawy uses Claude Code [hooks](https://docs.anthropic.com/en/docs/claude-code/hooks) to track session state. When Claude thinks, runs tools, finishes, or needs input, the hook scripts send status updates over WiFi to the device.

The device advertises itself as `clawy.local` via mDNS. No IP configuration needed.

### States

| Status | Display | Trigger |
|--------|---------|---------|
| READY | Idle with twinkling stars | Boot / session start |
| WORKING | Thinking with flowing waves | User sends a prompt |
| TOOL | Running with speed lines | Claude uses a tool (Read, Edit, Bash, etc.) |
| DONE | Jumping with confetti | Claude finishes responding |
| INPUT | Curious with pulsing glow | Claude asks a question |
| APPROVE | Alert with action lines | Claude needs permission |
| ERROR | Dizzy with screen shake | A tool call fails |
| SLEEPING | Curled up with fireflies | 30s idle |

### Physical Buttons

- **Button A**: Approve permission requests, skip boot animation
- **Button B**: Deny permission requests, toggle stats screen
- **Button B (long press)**: Enter demo mode
- **Button A + B (hold during boot)**: Reset WiFi credentials

## WiFi Setup

If you flashed via the [web flasher](https://clawy.lol/flash), enter your WiFi credentials on the setup page after flashing. If you built from source, either add credentials to `secrets.h` or the device will enter provisioning mode on first boot.

To reset WiFi, hold both buttons (A + B) during boot.

## Uninstall

```bash
./uninstall.sh
```

This removes the hooks from Claude Code settings and deletes `~/.clawy/`. If you added the `clawy` function to your shell profile, remove that line manually.

## Project Structure

```
clawy/
├── firmware/clawy/     Arduino sketch (M5StickC Plus 2)
├── hooks/              Claude Code hook scripts
├── assets/             Sprite exports and assets
├── install.sh          Hook installer
└── uninstall.sh        Hook uninstaller
```

## Roadmap

**Planned fixes:**
- Approval port authentication (shared secret token)
- Dependency checks in install/hook scripts
- WiFi provisioning colon-in-SSID fix

**Future:**
- Multi-session support (multiple Claude sessions, multiple devices)
- Sound effects via buzzer (approve alert, done chime, error buzz)
- OTA firmware updates from browser (no USB needed)
- Clawy levels up, lifetime stats unlock new animations
- Custom color themes and sprite sets
- Session history persisted across reboots
- Configurable audio notifications (walk away, hear when it needs you)
- Diagnostic script to test connectivity and verify hooks
- Device settings menu via button combo (brightness, WiFi, version)
- Voice input via built-in mic (hold to talk, local Whisper transcription)

## Security

Clawy communicates over plaintext TCP on your local network. The approval port (7801) — used for approving/denying Claude Code permission requests via the physical buttons — has no authentication. Anyone on the same network could send an approval response.

**Use Clawy on trusted networks only.** Token-based auth for the approval port is planned for a future release.

## Requirements

- **Device**: M5StickC Plus 2 (ESP32-PICO, 135x240 TFT, WiFi)
- **Build tools** (Path B only): Arduino CLI with `m5stack:esp32` core, M5Unified + M5GFX libraries
- **OS**: macOS or Linux (hooks use bash + python3)

## License

MIT
