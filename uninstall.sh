#!/bin/bash
# Clawy uninstaller — removes hooks and ~/.clawy directory
set -e

CLAWY_DIR="$HOME/.clawy"
SETTINGS_FILE="$HOME/.claude/settings.json"

# ── Colors ──────────────────────────────────────────────────
C_MAGENTA='\033[95m'
C_YELLOW='\033[93m'
C_WHITE='\033[97m'
C_PINK='\033[35m'
C_GREEN='\033[92m'
C_CYAN='\033[96m'
C_RED='\033[91m'
C_DIM='\033[2m'
C_BOLD='\033[1m'
C_RESET='\033[0m'

# ── ASCII Art Frames ────────────────────────────────────────
clawy_sad() {
  echo -e "${C_CYAN}     /\\_/\\  ${C_RESET}"
  echo -e "${C_CYAN}    ( ${C_RED}o${C_CYAN}.${C_RED}o${C_CYAN} ) ${C_RESET}"
  echo -e "${C_CYAN}     > ${C_PINK}^${C_CYAN} <  ${C_RESET}"
}

clawy_sleeping() {
  echo -e "${C_DIM}     /\\_/\\  ${C_RESET}"
  echo -e "${C_DIM}    ( -${C_CYAN}.${C_DIM}- ) ${C_RESET}"
  echo -e "${C_DIM}     > ${C_PINK}^${C_DIM} <  ${C_RESET}"
}

clawy_wave() {
  echo -e "${C_CYAN}     /\\_/\\  ${C_YELLOW}/${C_RESET}"
  echo -e "${C_CYAN}    ( ${C_GREEN}^${C_CYAN}.${C_GREEN}^${C_CYAN} )${C_RESET}"
  echo -e "${C_CYAN}     > ${C_PINK}^${C_CYAN} <  ${C_RESET}"
}

# ── Animation Helpers ───────────────────────────────────────
hide_cursor() { printf '\033[?25l'; }
show_cursor() { printf '\033[?25h'; }
move_up() { printf "\033[${1}A"; }

trap 'show_cursor' EXIT

spin_step() {
  local msg="$1"
  local frames=('.' '..' '...')
  for f in "${frames[@]}"; do
    printf "\r   ${C_DIM}%s%s${C_RESET}  " "$msg" "$f"
    sleep 0.2
  done
  printf "\r   ${C_GREEN}%s ${C_RESET}\n" "$msg done"
}

# ── Banner ──────────────────────────────────────────────────
echo ""
echo ""
hide_cursor
clawy_sad
show_cursor
echo ""
echo -e "  ${C_MAGENTA}C${C_YELLOW}l${C_WHITE}a${C_PINK}w${C_GREEN}y${C_RESET} ${C_DIM}uninstaller${C_RESET}"
echo -e "  ${C_DIM}────────────────────${C_RESET}"
echo ""

# ── Step 1: Remove Clawy hooks from settings.json ──────────
echo -e "  ${C_CYAN}1${C_RESET} Removing hooks"
if [ -f "$SETTINGS_FILE" ]; then
  python3 << 'PYEOF'
import json, os

settings_file = os.path.expanduser("~/.claude/settings.json")
try:
    with open(settings_file) as f:
        settings = json.load(f)
except (json.JSONDecodeError, IOError):
    print("   Could not read settings file, skipping.")
    exit(0)

hooks = settings.get("hooks", {})
changed = False
for event in list(hooks.keys()):
    before = len(hooks[event])
    hooks[event] = [
        e for e in hooks[event]
        if not any("/.clawy/" in h.get("command", "") for h in e.get("hooks", []))
    ]
    if len(hooks[event]) != before:
        changed = True
    if not hooks[event]:
        del hooks[event]

if not hooks:
    del settings["hooks"]

if changed:
    with open(settings_file, "w") as f:
        json.dump(settings, f, indent=2)

PYEOF
  spin_step "Hooks removed from settings.json"
else
  echo -e "   ${C_DIM}No settings file found, skipping${C_RESET}"
fi

# ── Step 2: Remove ~/.clawy directory ──────────────────────
echo -e "  ${C_CYAN}2${C_RESET} Cleaning up"
if [ -d "$CLAWY_DIR" ]; then
  rm -rf "$CLAWY_DIR"
fi
rm -f "/tmp/clawy-ip-$USER"
rmdir "/tmp/clawy-listener-$USER.lock" 2>/dev/null || true
spin_step "Removed ~/.clawy and temp files"

# ── Done ────────────────────────────────────────────────────
echo ""
echo -e "  ${C_DIM}────────────────────${C_RESET}"
echo ""

hide_cursor
clawy_sad; sleep 0.3; move_up 3
clawy_sleeping; sleep 0.4; move_up 3
clawy_wave; sleep 0.5; move_up 3
clawy_sleeping
show_cursor

echo ""
echo -e "  ${C_GREEN}${C_BOLD}Uninstall complete!${C_RESET}"
echo ""
echo -e "  ${C_DIM}If you added the clawy function to your shell profile,${C_RESET}"
echo -e "  ${C_DIM}remove that line manually.${C_RESET}"
echo ""
echo -e "  ${C_CYAN}See you next time!${C_RESET}"
echo ""
