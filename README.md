# Clawy

A JRPG-styled companion device for [Claude Code](https://docs.anthropic.com/en/docs/claude-code). A pixel art fox/cat lives on an M5StickC Plus 2 and reacts to your coding session in real-time — thinking, running, celebrating, or waiting for your input.

## What You Need

- **M5StickC Plus 2** (~$20) — [M5Stack store](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit)
- **Claude Code** installed and working
- **USB cable** (USB-C, data-capable)

## Quick Start

### Path A: Flash from Browser (Recommended)

1. Plug in the M5StickC Plus 2 via USB
2. Visit the [web flasher](https://marcusschiesser.github.io/clawy/) in Chrome or Edge
3. Click **Install Clawy** and select the serial port
4. Enter your WiFi SSID and password when prompted
5. Install the hooks:
   ```bash
   git clone https://github.com/marcvermeeren/clawy.git
   cd clawy
   ./install.sh
   ```
6. Start a session:
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

## WiFi Setup

### First-Time Setup

If you flashed via the web flasher, WiFi credentials are configured during the flash process using the Improv WiFi protocol.

If you built from source without `secrets.h`, the device enters provisioning mode — connect via a Web Serial-capable browser to configure WiFi.

### Reset WiFi

Hold both buttons (A + B) during boot to clear saved WiFi credentials. The device will re-enter provisioning mode.

## Uninstall

```bash
./uninstall.sh
```

This removes the hooks from Claude Code settings and deletes `~/.clawy/`. If you added the `clawy` alias to your shell profile, remove that line manually.

## Project Structure

```
clawy/
├── firmware/clawy/     Arduino sketch (M5StickC Plus 2)
├── hooks/              Claude Code hook scripts
├── web/                Browser-based firmware flasher
├── marketing/          Sprite exports and assets
├── install.sh          Hook installer
└── uninstall.sh        Hook uninstaller
```

## Requirements

- **Device**: M5StickC Plus 2 (ESP32-PICO, 135x240 TFT, WiFi)
- **Build tools** (Path B only): Arduino CLI with `m5stack:esp32` core, M5Unified + M5GFX libraries
- **OS**: macOS or Linux (hooks use bash + python3)

## License

MIT
