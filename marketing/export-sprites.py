#!/usr/bin/env python3
"""Export Clawy sprites from sprites.h to SVG stills, animated GIFs, and full state screens.

Usage:
    python3 export-sprites.py [--scale 8] [--out exports]

Outputs:
    exports/svg/          — individual SVG per sprite frame + paw icon
    exports/svg-nobg/     — same with transparent backgrounds
    exports/gif/          — animated GIF per animation cycle
    exports/sheets/       — sprite sheet PNGs per animation cycle
    exports/states/       — full state screen renders (PNG + animated GIF)
    exports/logo/         — boot screen title card (PNG + SVG)
"""

import re
import math
import argparse
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Pillow required: pip3 install Pillow")
    raise SystemExit(1)

# ── Config ──

SPRITES_H = Path(__file__).parent.parent / "sprites.h"

# Screen dimensions (matches TFT)
SCREEN_W = 135
SCREEN_H = 240

# Layout zones
HUD_Y, HUD_H = 0, 16
DIVIDER_Y = 17
FRAME_X, FRAME_Y, FRAME_W, FRAME_H = 4, 22, 127, 82
CHAR_CX, CHAR_CY = 67, FRAME_Y + 6 + 32
CHAR_X, CHAR_Y = CHAR_CX - 32, FRAME_Y + 6
TBOX_X, TBOX_Y, TBOX_W, TBOX_H = 4, 108, 127, 40
STATUS_Y, DETAIL_Y = 120, 138
BTN_Y = 210

# ── Colors (RGB tuples) ──

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
CYAN = (0, 255, 255)
GREEN = (0, 255, 0)
YELLOW = (255, 255, 0)
MAGENTA = (255, 0, 255)
ORANGE = (255, 180, 0)
RED = (255, 0, 0)
HUD_GRAY = (66, 66, 66)
DIM_GRAY = (49, 49, 49)
DARK_BG = (8, 8, 8)
PINK = (255, 105, 180)
HEART_RED = (255, 0, 0)

def dim(color, shift=1):
    return tuple(c >> shift for c in color)

def lerp_color(a, b, t):
    """Lerp between two RGB colors, t: 0.0-1.0."""
    t = max(0.0, min(1.0, t))
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

def in_frame(x, y):
    return x >= FRAME_X + 3 and x < FRAME_X + FRAME_W - 3 and \
           y >= FRAME_Y + 3 and y < FRAME_Y + FRAME_H - 3


# ── State definitions ──

STATES = {
    "READY":    {"color": WHITE,   "sprites": ["sprite_idle_1", "sprite_idle_2", "sprite_idle_blink", "sprite_idle_2"],
                 "label": "~ Clawy ~", "detail": "Awaiting orders...", "delay": 300},
    "WORKING":  {"color": CYAN,    "sprites": ["sprite_thinking_1", "sprite_thinking_2"],
                 "label": "Thinking...", "detail": "3s", "delay": 400},
    "TOOL":     {"color": GREEN,   "sprites": ["sprite_running_1", "sprite_running_2", "sprite_running_3", "sprite_running_2"],
                 "label": "Reading", "detail": "1s", "delay": 150},
    "DONE":     {"color": YELLOW,  "sprites": ["sprite_happy_1", "sprite_happy_2"],
                 "label": "Done!", "detail": None, "delay": 250},
    "INPUT":    {"color": MAGENTA, "sprites": ["sprite_curious_1", "sprite_curious_2"],
                 "label": "Need Input", "detail": None, "delay": 400},
    "APPROVE":  {"color": ORANGE,  "sprites": ["sprite_alert"],
                 "label": "Approve?", "detail": None, "delay": 500},
    "ERROR":    {"color": RED,     "sprites": ["sprite_dizzy_1", "sprite_dizzy_2"],
                 "label": "Error!", "detail": None, "delay": 300},
    "SLEEPING": {"color": DIM_GRAY,"sprites": ["sprite_sleeping_1", "sprite_sleeping_2"],
                 "label": "Sleeping...", "detail": None, "delay": 600},
}

# Animation cycles for bare sprite GIFs
ANIMATIONS = {
    "idle":     (["sprite_idle_1", "sprite_idle_2", "sprite_idle_blink", "sprite_idle_2"], WHITE, 300),
    "thinking": (["sprite_thinking_1", "sprite_thinking_2"], CYAN, 400),
    "running":  (["sprite_running_1", "sprite_running_2", "sprite_running_3", "sprite_running_2"], GREEN, 150),
    "happy":    (["sprite_happy_1", "sprite_happy_2"], YELLOW, 250),
    "curious":  (["sprite_curious_1", "sprite_curious_2"], MAGENTA, 400),
    "alert":    (["sprite_alert"], ORANGE, 500),
    "dizzy":    (["sprite_dizzy_1", "sprite_dizzy_2"], RED, 300),
    "sleeping": (["sprite_sleeping_1", "sprite_sleeping_2"], DIM_GRAY, 600),
}

SPRITE_COLORS = {
    "sprite_idle_1": "#FFFFFF", "sprite_idle_2": "#FFFFFF", "sprite_idle_blink": "#FFFFFF",
    "sprite_thinking_1": "#00FFFF", "sprite_thinking_2": "#00FFFF",
    "sprite_running_1": "#00FF00", "sprite_running_2": "#00FF00", "sprite_running_3": "#00FF00",
    "sprite_happy_1": "#FFFF00", "sprite_happy_2": "#FFFF00",
    "sprite_curious_1": "#FF00FF", "sprite_curious_2": "#FF00FF",
    "sprite_alert": "#FFA500",
    "sprite_dizzy_1": "#FF0000", "sprite_dizzy_2": "#FF0000",
    "sprite_sleeping_1": "#646464", "sprite_sleeping_2": "#646464",
    "icon_paw_16x16": "#FF69B4",
}

# Boot screen palettes (matching display.h)
STAR_HUES = [WHITE, (170, 220, 255), (255, 220, 100), (255, 180, 60),
             CYAN, (255, 200, 220), (180, 200, 255), (240, 200, 0)]
TITLE_GRAD = [CYAN, WHITE, MAGENTA, YELLOW, WHITE]
TITLE_PALETTE = [CYAN, WHITE, MAGENTA, YELLOW, WHITE, PINK, GREEN, ORANGE]

# Boot star data (matching bootStars[] in display.h)
BOOT_STARS = [
    (80, 30, 0), (-50, 70, 1), (20, -90, 0), (-80, -40, 2), (60, -60, 1),
    (-30, 85, 0), (90, -10, 2), (-70, -55, 1), (10, 95, 0), (75, 65, 2),
    (-95, 15, 0), (45, -80, 1), (-25, 60, 2), (85, 45, 0), (-60, -75, 1),
    (35, 90, 2), (-85, 25, 0), (55, -45, 1), (-15, -95, 0), (70, 50, 2),
    (-40, 80, 1), (95, -5, 0), (-55, -65, 2), (25, 75, 1), (-90, 35, 0),
    (50, -70, 2), (-65, 50, 1), (15, -85, 0), (65, 20, 2), (-45, -80, 1),
]


# ── Parse sprites ──

def parse_sprites(path: Path) -> dict:
    text = path.read_text()
    sprites = {}
    pattern = re.compile(
        r'const\s+uint8_t\s+(\w+)\[\]\s+PROGMEM\s*=\s*\{([^}]+)\}', re.DOTALL)
    for m in pattern.finditer(text):
        name = m.group(1)
        values = [int(x, 16) for x in re.findall(r'0x[0-9A-Fa-f]+', m.group(2))]
        sprites[name] = values
    return sprites


def bitmap_to_pixels(data, width, height):
    bpr = width // 8
    pixels = []
    for row in range(height):
        line = []
        for col in range(width):
            bi = row * bpr + (col // 8)
            bit = 7 - (col % 8)
            line.append(bool(data[bi] & (1 << bit)) if bi < len(data) else False)
        pixels.append(line)
    return pixels


# ── Drawing primitives (PIL-based, works at scale) ──

class Canvas:
    """Simple pixel canvas that mimics M5Canvas at a given scale."""

    def __init__(self, w, h, scale):
        self.w, self.h, self.s = w, h, scale
        self.img = Image.new('RGB', (w * scale, h * scale), BLACK)
        self.draw = ImageDraw.Draw(self.img)
        self._font = None
        self._load_font()

    def _load_font(self):
        try:
            self._font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", self.s * 6)
            self._font_sm = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", self.s * 4)
            self._font_lg = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", self.s * 10)
        except Exception:
            self._font = ImageFont.load_default()
            self._font_sm = self._font
            self._font_lg = self._font

    def pixel(self, x, y, color):
        if 0 <= x < self.w and 0 <= y < self.h:
            s = self.s
            self.draw.rectangle([x*s, y*s, x*s+s-1, y*s+s-1], fill=color)

    def fill_rect(self, x, y, w, h, color):
        s = self.s
        self.draw.rectangle([x*s, y*s, (x+w)*s-1, (y+h)*s-1], fill=color)

    def draw_rect(self, x, y, w, h, color):
        s = self.s
        # Top/bottom
        self.draw.rectangle([x*s, y*s, (x+w)*s-1, y*s+s-1], fill=color)
        self.draw.rectangle([x*s, (y+h-1)*s, (x+w)*s-1, (y+h)*s-1], fill=color)
        # Left/right
        self.draw.rectangle([x*s, y*s, x*s+s-1, (y+h)*s-1], fill=color)
        self.draw.rectangle([(x+w-1)*s, y*s, (x+w)*s-1, (y+h)*s-1], fill=color)

    def hline(self, x, y, w, color):
        self.fill_rect(x, y, w, 1, color)

    def vline(self, x, y, h, color):
        self.fill_rect(x, y, 1, h, color)

    def fill_circle(self, cx, cy, r, color):
        for dy in range(-r, r+1):
            for dx in range(-r, r+1):
                if dx*dx + dy*dy <= r*r:
                    self.pixel(cx+dx, cy+dy, color)

    def draw_line(self, x0, y0, x1, y1, color):
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        while True:
            self.pixel(x0, y0, color)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def draw_sprite(self, data, x, y, color, w=64, h=64):
        pixels = bitmap_to_pixels(data, w, h)
        for row in range(h):
            for col in range(w):
                if pixels[row][col]:
                    self.pixel(x + col, y + row, color)

    def draw_text_centered(self, text, cx, cy, color, font=None):
        f = font or self._font
        bbox = f.getbbox(text)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        s = self.s
        self.draw.text((cx*s - tw//2, cy*s - th//2), text, fill=color, font=f)


# ── Full screen rendering ──

def draw_heart(cv, x, y, color):
    cv.pixel(x+1, y, color); cv.pixel(x+3, y, color)
    cv.fill_rect(x, y+1, 5, 1, color)
    cv.fill_rect(x, y+2, 5, 1, color)
    for dx in [1,2,3]: cv.pixel(x+dx, y+3, color)
    cv.pixel(x+2, y+4, color)

def draw_hud(cv, accent):
    cv.fill_rect(0, HUD_Y, SCREEN_W, HUD_H, DARK_BG)
    for i in range(5):
        draw_heart(cv, 4 + i*7, 5, HEART_RED)
    cv.draw_text_centered("3:42", SCREEN_W//2, HUD_Y + HUD_H//2, HUD_GRAY, cv._font_sm)
    # WiFi dot
    cv.fill_circle(SCREEN_W - 27, 7, 2, GREEN)
    # Battery
    cv.draw_rect(SCREEN_W - 22, 4, 14, 7, HUD_GRAY)
    cv.fill_rect(SCREEN_W - 22 + 14, 6, 2, 3, HUD_GRAY)
    cv.fill_rect(SCREEN_W - 21, 5, 9, 5, GREEN)

def draw_divider(cv, color):
    cv.hline(0, DIVIDER_Y, SCREEN_W, color)
    cv.hline(0, DIVIDER_Y+1, SCREEN_W, color)

def draw_portrait_frame(cv, accent):
    d = dim(accent, 1)
    vd = dim(accent, 2)
    cv.draw_rect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, d)
    cv.draw_rect(FRAME_X+2, FRAME_Y+2, FRAME_W-4, FRAME_H-4, vd)
    bLen = 8
    for (fx, fy) in [(FRAME_X, FRAME_Y), (FRAME_X+FRAME_W-bLen, FRAME_Y),
                      (FRAME_X, FRAME_Y+FRAME_H-1), (FRAME_X+FRAME_W-bLen, FRAME_Y+FRAME_H-1)]:
        cv.hline(fx, fy if fy < FRAME_Y+FRAME_H//2 else fy, bLen, accent)
    for (fx, fy) in [(FRAME_X, FRAME_Y), (FRAME_X+FRAME_W-1, FRAME_Y),
                      (FRAME_X, FRAME_Y+FRAME_H-bLen), (FRAME_X+FRAME_W-1, FRAME_Y+FRAME_H-bLen)]:
        cv.vline(fx, fy, bLen, accent)

def draw_text_window(cv, accent):
    d = dim(accent, 1)
    vd = dim(accent, 2)
    cv.draw_rect(TBOX_X, TBOX_Y, TBOX_W, TBOX_H, d)
    cv.draw_rect(TBOX_X+2, TBOX_Y+2, TBOX_W-4, TBOX_H-4, vd)
    bLen = 6
    for (fx, fy) in [(TBOX_X, TBOX_Y), (TBOX_X+TBOX_W-bLen, TBOX_Y),
                      (TBOX_X, TBOX_Y+TBOX_H-1), (TBOX_X+TBOX_W-bLen, TBOX_Y+TBOX_H-1)]:
        cv.hline(fx, fy, bLen, d)
    for (fx, fy) in [(TBOX_X, TBOX_Y), (TBOX_X+TBOX_W-1, TBOX_Y),
                      (TBOX_X, TBOX_Y+TBOX_H-bLen), (TBOX_X+TBOX_W-1, TBOX_Y+TBOX_H-bLen)]:
        cv.vline(fx, fy, bLen, d)

def draw_button_bar(cv):
    cv.draw_text_centered("[A: Yes!]  [B: Nope]", SCREEN_W//2, BTN_Y + 10, ORANGE, cv._font_sm)

# ── Background effects ──

def draw_bg_starfield(cv, frame, color):
    d = dim(color, 2)
    for i in range(12):
        x = FRAME_X + 5 + ((i * 37 + 13) % (FRAME_W - 10))
        y = FRAME_Y + 5 + ((i * 23 + 7) % (FRAME_H - 10))
        if in_frame(x, y):
            cv.pixel(x, y, color if (frame + i) % 5 < 3 else d)

def draw_bg_wave(cv, frame, color):
    d = dim(color, 2)
    sin_lut = [0, 2, 3, 2, 0, -2, -3, -2]
    for row in range(4):
        y = FRAME_Y + 15 + row * 18
        for col in range(0, FRAME_W - 10, 4):
            phase = (col // 4 + frame + row * 2) % 8
            x = FRAME_X + 5 + col
            dy = y + sin_lut[phase]
            if in_frame(x, dy):
                cv.pixel(x, dy, d)

def draw_bg_speed_lines(cv, frame, color):
    d = dim(color, 2)
    for i in range(6):
        y = FRAME_Y + 8 + i * 12
        offset = (frame * 3 + i * 7) % 20
        x = FRAME_X + FRAME_W - 6 - offset
        while x > FRAME_X + 4:
            dash = 5 + (i % 3) * 2
            for dd in range(dash):
                if in_frame(x - dd, y):
                    cv.pixel(x - dd, y, d)
            x -= 20

def draw_bg_confetti(cv, frame, color):
    for i in range(8):
        x = FRAME_X + 6 + ((i * 41 + 11) % (FRAME_W - 12))
        y = FRAME_Y + 5 + ((i * 19 + frame * 2) % (FRAME_H - 10))
        c = color if i % 2 == 0 else dim(color, 1)
        if in_frame(x, y) and in_frame(x+1, y+1):
            cv.fill_rect(x, y, 2, 2, c)

def draw_bg_pulse(cv, frame, color):
    d = dim(color, 3)
    base_r = 28 + ((frame % 8 < 4) * (frame % 4) + (frame % 8 >= 4) * (4 - frame % 4))
    sin_lut = [0, 3, 5, 7, 8, 7, 5, 3, 0, -3, -5, -7, -8, -7, -5, -3]
    cos_lut = [8, 7, 5, 3, 0, -3, -5, -7, -8, -7, -5, -3, 0, 3, 5, 7]
    for r in range(base_r, 10, -8):
        for a in range(0, 32):
            idx = a % 16
            px = CHAR_CX + (r * cos_lut[idx]) // 8
            py = CHAR_CY + (r * sin_lut[idx]) // 8
            if in_frame(px, py):
                cv.pixel(px, py, d)

def draw_bg_radial(cv, frame, color):
    d = dim(color, 2)
    sin_lut = [0, 71, 100, 71, 0, -71, -100, -71]
    cos_lut = [100, 71, 0, -71, -100, -71, 0, 71]
    for i in range(8):
        angle = (i * 45 + frame * 15) % 360
        idx = (angle // 45) % 8
        for r in range(25, 38, 2):
            px = CHAR_CX + (r * cos_lut[idx]) // 100
            py = CHAR_CY + (r * sin_lut[idx]) // 100
            if in_frame(px, py):
                cv.pixel(px, py, d)

def draw_bg_fireflies(cv, frame, color):
    d = dim(color, 1)
    drift = [0, 1, 1, 0, -1, -1]
    for i in range(5):
        bx = FRAME_X + 10 + ((i * 31 + 5) % (FRAME_W - 20))
        by = FRAME_Y + 10 + ((i * 17 + 3) % (FRAME_H - 20))
        phase = (frame // 2 + i) % 6
        x = bx + drift[phase]
        y = by + drift[(phase + 2) % 6]
        if in_frame(x, y):
            if (frame + i * 3) % 8 < 2:
                cv.pixel(x, y, color)
                for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
                    cv.pixel(x+dx, y+dy, d)
            else:
                cv.pixel(x, y, d)

BG_FUNCS = {
    "READY": draw_bg_starfield, "WORKING": draw_bg_wave, "TOOL": draw_bg_speed_lines,
    "DONE": draw_bg_confetti, "INPUT": draw_bg_pulse, "APPROVE": draw_bg_radial,
    "ERROR": lambda cv, f, c: None, "SLEEPING": draw_bg_fireflies,
}

# ── Particle effects ──

def draw_sparkle(cv, x, y, color):
    cv.pixel(x, y, color)
    for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
        cv.pixel(x+dx, y+dy, color)

def draw_sparkles(cv, frame, color):
    sx = [CHAR_X-6, CHAR_X+64+4, CHAR_X+12, CHAR_X+64-8]
    sy = [CHAR_Y+12, CHAR_Y+8, CHAR_Y-4, CHAR_Y+64-8]
    for i in range(4):
        if (frame+i)%3 != 0 and in_frame(sx[i], sy[i]):
            draw_sparkle(cv, sx[i], sy[i], color)

def draw_thought_dots(cv, frame, color):
    for i in range(3):
        x = CHAR_CX - 8 + i * 8
        y = CHAR_Y - 6 - ((frame + i) % 4)
        r = 3 if (frame+i)%3 == 0 else 2
        if in_frame(x, y):
            cv.fill_circle(x, y, r, color)

def draw_dust_puffs(cv, frame, color):
    for i in range(3):
        x = CHAR_X - 4 - (frame % 4) * 2 - i * 6
        y = CHAR_Y + 64 - 8 + ((i + frame) % 3) - 1
        r = 3 - i
        if r > 0 and in_frame(x, y):
            cv.fill_circle(x, y, r, color)

def draw_bouncing_q(cv, frame, color):
    bounce = [0, -2, -4, -5, -4, -2, 0, 1]
    x = CHAR_X + 64 + 6
    y = CHAR_Y + 12 + bounce[frame % 8]
    if in_frame(x, y):
        cv.draw_text_centered("?", x, y, color, cv._font)

def draw_pulsing_bang(cv, frame, color):
    if (frame % 4) < 2:
        x = CHAR_X + 64 + 6
        y = CHAR_Y + 14
        if in_frame(x, y):
            cv.draw_text_centered("!", x, y, color, cv._font)

def draw_circling_stars(cv, frame, color):
    cos_lut = [100, 71, 0, -71, -100, -71, 0, 71]
    sin_lut = [0, 71, 100, 71, 0, -71, -100, -71]
    for i in range(3):
        angle = (frame * 30 + i * 120) % 360
        idx = (angle // 45) % 8
        sx = CHAR_CX + (16 * cos_lut[idx]) // 100
        sy = (CHAR_Y - 2) + (16 * sin_lut[idx]) // 100
        if in_frame(sx, sy):
            cv.pixel(sx, sy, color)
            for dx in [-1, 1]:
                for dy in [-1, 1]:
                    cv.pixel(sx+dx, sy+dy, color)

def draw_zzz(cv, frame, color):
    bx = CHAR_CX + 20
    for i in range(3):
        phase = (frame + i * 3) % 12
        x = bx + i * 4
        y = CHAR_Y - 3 - phase * 2
        if CHAR_Y - 25 < y < CHAR_Y and in_frame(x, y):
            cv.draw_text_centered("Z", x, y, color, cv._font_sm)

PARTICLE_FUNCS = {
    "READY": lambda cv, f, c: None,
    "WORKING": draw_thought_dots,
    "TOOL": draw_dust_puffs,
    "DONE": draw_sparkles,
    "INPUT": draw_bouncing_q,
    "APPROVE": draw_pulsing_bang,
    "ERROR": draw_circling_stars,
    "SLEEPING": draw_zzz,
}


def render_state_frame(sprites, state_name, frame_idx, scale):
    """Render one full state screen frame at the given scale."""
    st = STATES[state_name]
    accent = st["color"]
    sprite_names = st["sprites"]
    sprite_name = sprite_names[frame_idx % len(sprite_names)]
    sprite_data = sprites.get(sprite_name)

    cv = Canvas(SCREEN_W, SCREEN_H, scale)

    # HUD
    draw_hud(cv, accent)
    draw_divider(cv, accent)

    # Portrait frame
    draw_portrait_frame(cv, accent)

    # Background effect
    bg_func = BG_FUNCS.get(state_name, lambda c,f,col: None)
    bg_func(cv, frame_idx, accent)

    # Character sprite
    if sprite_data:
        cv.draw_sprite(sprite_data, CHAR_X, CHAR_Y, accent)

    # Particles
    part_func = PARTICLE_FUNCS.get(state_name, lambda c,f,col: None)
    part_func(cv, frame_idx, accent)

    # Text window
    draw_text_window(cv, accent)

    # Status text
    cv.draw_text_centered(st["label"], SCREEN_W//2, STATUS_Y, accent)

    # Detail text
    if st["detail"]:
        cv.draw_text_centered(st["detail"], SCREEN_W//2, DETAIL_Y, HUD_GRAY, cv._font_sm)

    # Button bar for APPROVE
    if state_name == "APPROVE":
        draw_button_bar(cv)

    return cv.img


# ── Boot screen logo ──

def render_boot_logo(sprites, scale):
    """Render the boot title screen (phase 4 idle state) as a logo."""
    cv = Canvas(SCREEN_W, SCREEN_H, scale)
    cx = SCREEN_W // 2
    cy = SCREEN_H // 2 - 20

    # Nebula background
    cv.fill_circle(cx, cy, 80, dim(MAGENTA, 5))
    cv.fill_circle(cx - 35, cy + 20, 44, dim(MAGENTA, 3))
    cv.fill_circle(cx + 30, cy - 25, 38, dim(CYAN, 3))
    cv.fill_circle(cx + 10, cy + 50, 33, dim((0, 0, 128), 2))
    cv.fill_circle(cx - 20, cy - 40, 30, dim(CYAN, 4))
    cv.fill_circle(cx + 40, cy + 35, 30, dim(MAGENTA, 4))

    # Twinkling colored stars
    for i, (dx, dy, br) in enumerate(BOOT_STARS):
        x = cx + int(dx * 1.5)
        y = cy + int(dy * 1.5)
        if 0 <= x < SCREEN_W and 0 <= y < SCREEN_H:
            bright = 0.7 * (0.5 + br * 0.25)
            col = lerp_color(BLACK, STAR_HUES[i % 8], bright)
            cv.pixel(x, y, col)

    # Paw icon 2x (32x32) at y=45
    paw_data = sprites.get("icon_paw_16x16")
    if paw_data:
        paw_pixels = bitmap_to_pixels(paw_data, 16, 16)
        for row in range(16):
            for col in range(16):
                if paw_pixels[row][col]:
                    cv.fill_rect(cx - 16 + col*2, 45 + row*2, 2, 2, PINK)

    # Title "Clawy" with per-char colors + drop shadow
    cv.draw_text_centered("Clawy", cx + 1, 101, dim(CYAN, 3), cv._font_lg)  # shadow
    cv.draw_text_centered("Clawy", cx, 100, CYAN, cv._font_lg)

    # Tagline
    tag_col = dim(CYAN, 2)
    cv.draw_text_centered("a claude code", cx, 128, tag_col, cv._font_sm)
    cv.draw_text_centered("companion", cx, 140, tag_col, cv._font_sm)

    # PRESS START
    cv.draw_text_centered("- PRESS START -", cx, 170, WHITE, cv._font_sm)

    # CONNECTED
    cv.draw_text_centered("CONNECTED", cx, 215, dim(GREEN, 1), cv._font_sm)

    return cv.img


# ── SVG helpers ──

def pixels_to_svg(pixels, width, height, scale, color="#FFFFFF", bg="#000000"):
    sw, sh = width * scale, height * scale
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{sw}" height="{sh}" '
        f'viewBox="0 0 {width} {height}" shape-rendering="crispEdges">',
        f'<rect width="{width}" height="{height}" fill="{bg}"/>',
    ]
    for y, row in enumerate(pixels):
        x = 0
        while x < width:
            if row[x]:
                run = 1
                while x + run < width and row[x + run]:
                    run += 1
                parts.append(f'<rect x="{x}" y="{y}" width="{run}" height="1" fill="{color}"/>')
                x += run
            else:
                x += 1
    parts.append('</svg>')
    return '\n'.join(parts)


def pixels_to_image(pixels, width, height, scale, color=(255,255,255), bg=(0,0,0)):
    img = Image.new('RGBA', (width * scale, height * scale), (*bg, 255))
    for y, row in enumerate(pixels):
        for x, on in enumerate(row):
            if on:
                for dy in range(scale):
                    for dx in range(scale):
                        img.putpixel((x*scale+dx, y*scale+dy), (*color, 255))
    return img


# ── Export functions ──

def export_svgs(sprites, out_dir, scale):
    for sub, bg in [("svg", "#000000"), ("svg-nobg", "none")]:
        d = out_dir / sub
        d.mkdir(parents=True, exist_ok=True)
        for name, data in sprites.items():
            if name.startswith("icon_paw"):
                w, h = 16, 16
            elif name.startswith("sprite_"):
                w, h = 64, 64
            else:
                continue
            color = SPRITE_COLORS.get(name, "#FFFFFF")
            pixels = bitmap_to_pixels(data, w, h)
            svg = pixels_to_svg(pixels, w, h, scale, color=color, bg=bg)
            (d / f"{name}.svg").write_text(svg)
        if sub == "svg":
            print(f"  {len([n for n in sprites if n.startswith(('sprite_','icon_'))])} SVGs")
    print(f"  + transparent versions in svg-nobg/")


def export_gifs(sprites, out_dir, scale):
    gif_dir = out_dir / "gif"
    gif_dir.mkdir(parents=True, exist_ok=True)
    for name, (frames, color, delay) in ANIMATIONS.items():
        imgs = []
        for fn in frames:
            if fn in sprites:
                pixels = bitmap_to_pixels(sprites[fn], 64, 64)
                imgs.append(pixels_to_image(pixels, 64, 64, scale, color=color))
        if imgs:
            path = gif_dir / f"{name}.gif"
            imgs[0].save(path, save_all=True, append_images=imgs[1:],
                         duration=delay, loop=0, disposal=2)
            print(f"  GIF: {path.name} ({len(imgs)} frames)")

    # Paw color cycle
    if "icon_paw_16x16" in sprites:
        paw_px = bitmap_to_pixels(sprites["icon_paw_16x16"], 16, 16)
        colors = [PINK, (255,140,100), ORANGE, (255,200,50), ORANGE, (255,140,100)]
        paw_imgs = [pixels_to_image(paw_px, 16, 16, scale, color=c) for c in colors]
        path = gif_dir / "paw_cycle.gif"
        paw_imgs[0].save(path, save_all=True, append_images=paw_imgs[1:],
                         duration=200, loop=0, disposal=2)
        print(f"  GIF: paw_cycle.gif ({len(paw_imgs)} frames)")


def export_sheets(sprites, out_dir, scale):
    sheet_dir = out_dir / "sheets"
    sheet_dir.mkdir(parents=True, exist_ok=True)
    for name, (frames, color, _) in ANIMATIONS.items():
        valid = [f for f in frames if f in sprites]
        if not valid:
            continue
        sw, sh = 64 * scale, 64 * scale
        sheet = Image.new('RGBA', (sw * len(valid), sh), (0,0,0,0))
        for i, fn in enumerate(valid):
            pixels = bitmap_to_pixels(sprites[fn], 64, 64)
            sheet.paste(pixels_to_image(pixels, 64, 64, scale, color=color, bg=BLACK), (i*sw, 0))
        path = sheet_dir / f"{name}_sheet.png"
        sheet.save(path)
        print(f"  Sheet: {path.name} ({len(valid)} frames)")


def export_states(sprites, out_dir, scale):
    """Export full state screen renders as PNGs and animated GIFs."""
    state_dir = out_dir / "states"
    state_dir.mkdir(parents=True, exist_ok=True)

    for state_name, st in STATES.items():
        n_frames = len(st["sprites"])
        # Render enough frames to show a full cycle
        cycle_len = max(n_frames, 4)  # at least 4 frames for visual interest

        imgs = []
        for f in range(cycle_len):
            img = render_state_frame(sprites, state_name, f, scale)
            imgs.append(img)

        # Save first frame as PNG
        png_path = state_dir / f"{state_name.lower()}.png"
        imgs[0].save(png_path)

        # Save animated GIF
        gif_path = state_dir / f"{state_name.lower()}.gif"
        imgs[0].save(gif_path, save_all=True, append_images=imgs[1:],
                     duration=st["delay"], loop=0, disposal=2)

        print(f"  {state_name}: PNG + GIF ({cycle_len} frames, {st['delay']}ms)")


def export_logo(sprites, out_dir, scale):
    """Export boot screen title card."""
    logo_dir = out_dir / "logo"
    logo_dir.mkdir(parents=True, exist_ok=True)

    img = render_boot_logo(sprites, scale)
    png_path = logo_dir / "clawy_title.png"
    img.save(png_path)
    print(f"  PNG: clawy_title.png ({img.width}x{img.height})")

    # Also save a cropped version (just the logo area, no HUD/buttons)
    s = scale
    crop_top = 30 * s
    crop_bottom = 155 * s
    cropped = img.crop((0, crop_top, SCREEN_W * s, crop_bottom))
    crop_path = logo_dir / "clawy_logo_cropped.png"
    cropped.save(crop_path)
    print(f"  PNG: clawy_logo_cropped.png ({cropped.width}x{cropped.height})")


def main():
    parser = argparse.ArgumentParser(description="Export Clawy sprites, state screens, and logo")
    parser.add_argument("--scale", type=int, default=4, help="Pixel scale factor (default: 4)")
    parser.add_argument("--out", type=str, default="exports", help="Output directory")
    args = parser.parse_args()

    out_dir = Path(__file__).parent / args.out
    print(f"Exporting (scale={args.scale}x) → {out_dir}/\n")

    sprites = parse_sprites(SPRITES_H)
    print(f"Parsed {len(sprites)} sprites from sprites.h\n")

    print("── SVGs ──")
    export_svgs(sprites, out_dir, args.scale)

    print("\n── Sprite GIFs ──")
    export_gifs(sprites, out_dir, args.scale)

    print("\n── Sprite Sheets ──")
    export_sheets(sprites, out_dir, args.scale)

    print("\n── Full State Screens ──")
    export_states(sprites, out_dir, args.scale)

    print("\n── Boot Logo ──")
    export_logo(sprites, out_dir, args.scale)

    print(f"\nDone! All exports in {out_dir}/")


if __name__ == "__main__":
    main()
