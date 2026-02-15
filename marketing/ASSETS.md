# Clawy — Marketing Assets Reference

## What Is Clawy

Clawy is a physical desk companion for Claude Code sessions. It's a pixel-art fox/cat character that lives on a tiny 1.14" color TFT display (M5StickC Plus 2, 135x240px portrait). It connects over WiFi and shows real-time session state with JRPG-styled visuals: animated character sprites, per-state background effects, dialog-box text windows, and a colorful boot sequence.

The aesthetic is **SNES title screen meets tamagotchi** — pixel art, vibrant colors on black, double-bordered JRPG frames, and retro text.

---

## Fonts

Three typefaces are used. All are free/open-source.

### Primary: GNU FreeSans Bold
- **Used for:** The "Clawy" title on the boot screen (18pt), stats screen title (9pt), particle text like "?" and "!" (9pt)
- **Download:** https://www.gnu.org/software/freefont/ — file is `FreeSansBold.ttf`
- **License:** GPL with font exception (free to use, embed, distribute)
- **Web alternative:** `font-family: 'FreeSans', 'Helvetica Neue', Arial, sans-serif` — or host the TTF as a `@font-face`
- **Character:** Clean, bold, sans-serif. Reads well at small sizes on the TFT.

### Secondary: Pixel Bitmap (Font0 / GLCD 5x7)
- **Used for:** All small UI text — HUD timer, tagline ("a claude code companion"), PRESS START, detail lines, status labels
- **What it is:** A classic 5x7 pixel bitmap font (monospaced, all-caps feels retro). This is the built-in GLCD font from the Adafruit GFX / M5GFX library.
- **Web equivalent:** Use **Press Start 2P** from Google Fonts (https://fonts.google.com/specimen/Press+Start+2P) for the same retro pixel feel. Alternatives: **Silkscreen**, **04b03**, **VT323**.
- **Character:** Chunky pixel grid, monospaced, 8-bit arcade energy.

### Tertiary: BMPfont (Font2)
- **Used for:** Status text inside the dialog box ("Done!", "Thinking...", "Approve?")
- **What it is:** A medium-sized bitmap proportional font from the M5GFX library. Slightly larger than Font0.
- **Web equivalent:** Same as Font0 — use **Press Start 2P** or any pixel font at a slightly larger size (14-16px vs 10-12px).

### Font Usage Summary for the Landing Page

| Element | Device Font | Web Recommendation |
|---------|------------|-------------------|
| Hero title "Clawy" | FreeSans Bold 18pt | FreeSans Bold or Helvetica Bold, large (48-72px) |
| Section headings | FreeSans Bold 9pt | FreeSans Bold, medium (24-32px) |
| Body / descriptions | N/A (not on device) | Any clean sans-serif |
| Pixel UI overlays, labels | GLCD 5x7 bitmap | Press Start 2P at 10-14px |
| Status text in mockups | BMPfont bitmap | Press Start 2P at 14-16px |

---

## Color Palette

All colors are defined as RGB565 on the device. Here they are in hex RGB for web use.

### Core Colors

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| Black | `#000000` | 0, 0, 0 | Background everywhere |
| White | `#FFFFFF` | 255, 255, 255 | READY state, default text |
| Cyan | `#00FFFF` | 0, 255, 255 | WORKING state, boot title gradient |
| Green | `#00FF00` | 0, 255, 0 | TOOL state, WiFi indicator, battery |
| Yellow | `#FFE000` | 255, 224, 0 | DONE state |
| Magenta | `#FF00FF` | 255, 0, 255 | INPUT state, nebula glow |
| Orange | `#FFB400` | 255, 180, 0 | APPROVE state |
| Red | `#FF0000` | 255, 0, 0 | ERROR state, hearts |
| Pink | `#FF69B4` | 255, 105, 180 | Paw icon, boot screen accent |
| HUD Gray | `#424242` | 66, 66, 66 | Timer text, detail text, borders |
| Dark BG | `#080808` | 8, 8, 8 | HUD bar background |

### State Color Map

| State | Accent Color | Meaning |
|-------|-------------|---------|
| READY | White | Idle, awaiting commands |
| WORKING | Cyan | Claude is thinking |
| TOOL | Green | Claude is using a tool |
| DONE | Yellow | Response complete |
| INPUT | Magenta | Waiting for user input |
| APPROVE | Orange | Permission needed |
| ERROR | Red | Something went wrong |
| SLEEPING | Dim Gray | Auto-sleep after inactivity |

### Boot Screen Palette

The boot/title screen uses additional colors for the nebula, stars, and title gradient:

| Element | Colors |
|---------|--------|
| Nebula clouds | Dim magenta, dim cyan, deep blue |
| Star hues | White, light blue, warm yellow, amber, cyan, light pink, pale blue, gold |
| Title gradient ("Clawy") | Cycles through: cyan, white, magenta, yellow, white, pink, green, orange |
| Paw icon | Cycles: pink → orange → yellow |
| PRESS START | Pulses: white → cyan → yellow |
| Tagline | Dim cyan |

---

## Asset Inventory

All assets are in `marketing/exports/`. Generated at 4x scale by default (re-run `export-sprites.py --scale 8` for higher res).

### `exports/logo/`
Boot screen title card — use as the hero image / logo.

| File | Size | Description |
|------|------|-------------|
| `clawy_title.png` | 540x960 | Full boot screen (nebula bg, 2x paw, "Clawy" title, tagline, PRESS START, CONNECTED) |
| `clawy_logo_cropped.png` | 540x500 | Cropped to just the logo area (paw + title + tagline, no HUD/buttons) |

### `exports/states/`
Full JRPG screen renders for each state — includes HUD bar (hearts, timer, WiFi dot, battery), portrait frame with corner brackets, animated background effect, character sprite, particle effects, and text window with status label.

| File | State | Accent | Sprite | Background | Particles |
|------|-------|--------|--------|------------|-----------|
| `ready.png` / `ready.gif` | READY | White | Standing idle (tail wag + blink) | Twinkling starfield | None |
| `working.png` / `working.gif` | WORKING | Cyan | Sitting, looking around | Flowing sine wave | Thought dots (3 circles) |
| `tool.png` / `tool.gif` | TOOL | Green | Running trot cycle | Speed lines (dashes) | Dust puffs |
| `done.png` / `done.gif` | DONE | Yellow | Jumping with joy | Falling confetti | Sparkles (cross shape) |
| `input.png` / `input.gif` | INPUT | Magenta | Curious head tilt | Pulsing glow rings | Bouncing "?" |
| `approve.png` / `approve.gif` | APPROVE | Orange | Alert, paw raised | Rotating action lines | Pulsing "!" |
| `error.png` / `error.gif` | ERROR | Red | Dizzy, fallen over | (screen shake on device) | Circling stars |
| `sleeping.png` / `sleeping.gif` | SLEEPING | Dim Gray | Curled up breathing | Drifting fireflies | Floating "Z Z Z" |

GIFs are looping animations (2-4 frames each). PNGs are single frame stills.

### `exports/gif/`
Bare sprite animations on black background (no UI frame). Good for inline use, hover effects, or small accents.

| File | Frames | Delay | Color |
|------|--------|-------|-------|
| `idle.gif` | 4 | 300ms | White |
| `thinking.gif` | 2 | 400ms | Cyan |
| `running.gif` | 4 | 150ms | Green |
| `happy.gif` | 2 | 250ms | Yellow |
| `curious.gif` | 2 | 400ms | Magenta |
| `alert.gif` | 1 | 500ms | Orange |
| `dizzy.gif` | 2 | 300ms | Red |
| `sleeping.gif` | 2 | 600ms | Dim Gray |
| `paw_cycle.gif` | 6 | 200ms | Pink → Orange → Yellow cycle |

### `exports/svg/` and `exports/svg-nobg/`
Individual SVG per sprite frame. `svg/` has black backgrounds, `svg-nobg/` has transparent backgrounds. Each SVG uses `shape-rendering="crispEdges"` and a viewBox matching the native pixel grid — scales cleanly to any size.

18 files total: 17 character sprites + 1 paw icon.

Sprites are named by state and frame: `sprite_idle_1.svg`, `sprite_running_3.svg`, `sprite_happy_2.svg`, etc.

Colors match the state accent (white for idle, cyan for thinking, green for running, etc.). For the landing page, the SVGs can be recolored with CSS (`fill` override or CSS `filter`).

### `exports/sheets/`
Sprite sheet PNGs for CSS `steps()` animation. Each sheet has all frames of one animation laid out horizontally.

| File | Frames | Dimensions |
|------|--------|------------|
| `idle_sheet.png` | 4 | 1024x256 |
| `thinking_sheet.png` | 2 | 512x256 |
| `running_sheet.png` | 4 | 1024x256 |
| `happy_sheet.png` | 2 | 512x256 |
| `curious_sheet.png` | 2 | 512x256 |
| `alert_sheet.png` | 1 | 256x256 |
| `dizzy_sheet.png` | 2 | 512x256 |
| `sleeping_sheet.png` | 2 | 512x256 |

CSS animation example:
```css
.sprite-running {
  width: 256px;
  height: 256px;
  background: url('running_sheet.png') left center;
  animation: run 0.6s steps(4) infinite;
}
@keyframes run {
  to { background-position: -1024px center; }
}
```

---

## Visual Style Notes for Landing Page

- **Dark theme.** The device is black with colored accents — the landing page should match. Deep black or near-black (`#0a0a0a`) background.
- **Pixel-perfect rendering.** All sprites are pixel art. Use `image-rendering: pixelated` on all sprite/GIF images to prevent anti-aliasing blur at larger sizes.
- **Color-on-black contrast.** Each state has one bright accent color on black. This high-contrast look is core to the aesthetic.
- **JRPG frame borders.** The double-border frames with corner bracket accents are a signature visual. Consider using CSS borders or SVG to recreate them for card/section containers.
- **The boot screen is the hero.** The nebula background with colored stars, big pink paw, rainbow "Clawy" title, and "PRESS START" is the most visually rich asset — use `clawy_title.png` or recreate it with CSS/canvas for the hero section.
- **Retro-modern blend.** Pixel fonts for UI labels but clean sans-serif for body copy. The device mixes both and the landing page should too.

---

## Re-Generating Assets

```bash
cd marketing/
python3 export-sprites.py --scale 4 --out exports    # default (540x960 states)
python3 export-sprites.py --scale 8 --out exports-hd  # high-res (1080x1920 states)
```

Requires Python 3 + Pillow (`pip3 install Pillow`). Reads directly from `../sprites.h`.
