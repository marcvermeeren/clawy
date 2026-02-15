#!/bin/bash
# Clawy installer — sets up Claude Code hooks and optional shell alias
set -e

CLAWY_DIR="$HOME/.clawy"
HOOKS_DIR="$CLAWY_DIR/hooks"
SETTINGS_FILE="$HOME/.claude/settings.json"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ── Colors ──────────────────────────────────────────────────
C_MAGENTA='\033[95m'
C_YELLOW='\033[93m'
C_WHITE='\033[97m'
C_PINK='\033[35m'
C_GREEN='\033[92m'
C_CYAN='\033[96m'
C_DIM='\033[2m'
C_BOLD='\033[1m'
C_RESET='\033[0m'

# ── ASCII Art Frames ────────────────────────────────────────
clawy_idle() {
  echo -e "${C_CYAN}     /\\_/\\  ${C_RESET}"
  echo -e "${C_CYAN}    ( ${C_WHITE}o${C_CYAN}.${C_WHITE}o${C_CYAN} ) ${C_RESET}"
  echo -e "${C_CYAN}     > ${C_PINK}^${C_CYAN} <  ${C_RESET}"
}

clawy_blink() {
  echo -e "${C_CYAN}     /\\_/\\  ${C_RESET}"
  echo -e "${C_CYAN}    ( ${C_YELLOW}-${C_CYAN}.${C_YELLOW}-${C_CYAN} ) ${C_RESET}"
  echo -e "${C_CYAN}     > ${C_PINK}^${C_CYAN} <  ${C_RESET}"
}

clawy_happy() {
  echo -e "${C_CYAN}     /\\_/\\  ${C_RESET}"
  echo -e "${C_CYAN}    ( ${C_GREEN}^${C_CYAN}.${C_GREEN}^${C_CYAN} ) ${C_RESET}"
  echo -e "${C_CYAN}     > ${C_PINK}^${C_CYAN} <  ${C_RESET}"
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

# Restore cursor on exit
trap 'show_cursor' EXIT

animate_clawy() {
  hide_cursor
  # Frame 1: idle
  clawy_idle; sleep 0.3; move_up 3
  # Frame 2: blink
  clawy_blink; sleep 0.15; move_up 3
  # Frame 3: idle
  clawy_idle; sleep 0.3; move_up 3
  # Frame 4: idle
  clawy_idle; sleep 0.2; move_up 3
  # Frame 5: blink
  clawy_blink; sleep 0.15; move_up 3
  # Frame 6: happy
  clawy_happy; sleep 0.4; move_up 3
  # Final: idle
  clawy_idle
  show_cursor
}

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
clear_line() { printf '\r\033[K'; }

echo ""
echo ""
animate_clawy
echo ""
echo -e "  ${C_MAGENTA}C${C_YELLOW}l${C_WHITE}a${C_PINK}w${C_GREEN}y${C_RESET} ${C_DIM}installer${C_RESET}"
echo -e "  ${C_DIM}────────────────────${C_RESET}"
echo ""

# ── Step 1: Copy hooks ─────────────────────────────────────
echo -e "  ${C_CYAN}1${C_RESET} Installing hooks"
mkdir -p "$HOOKS_DIR"
cp "$SCRIPT_DIR/hooks/send-status.sh" "$HOOKS_DIR/"
cp "$SCRIPT_DIR/hooks/send-tool-status.sh" "$HOOKS_DIR/"
cp "$SCRIPT_DIR/hooks/permission-listener.sh" "$HOOKS_DIR/"
chmod +x "$HOOKS_DIR/send-status.sh" "$HOOKS_DIR/send-tool-status.sh" "$HOOKS_DIR/permission-listener.sh"
spin_step "Hooks copied to ~/.clawy/hooks"

# ── Step 2: Merge hooks into settings.json ─────────────────
echo -e "  ${C_CYAN}2${C_RESET} Configuring Claude Code"
mkdir -p "$HOME/.claude"

python3 << 'PYEOF'
import json, os

settings_file = os.path.expanduser("~/.claude/settings.json")
hooks_dir = os.path.expanduser("~/.clawy/hooks")

clawy_hooks = {
    "Notification": [
        {
            "matcher": "idle_prompt",
            "hooks": [{"type": "command", "command": hooks_dir + "/send-status.sh INPUT"}]
        }
    ],
    "PermissionRequest": [
        {
            "hooks": [{"type": "command", "command": hooks_dir + "/permission-listener.sh"}]
        }
    ],
    "UserPromptSubmit": [
        {
            "hooks": [{"type": "command", "command": hooks_dir + "/send-status.sh WORKING"}]
        }
    ],
    "PreToolUse": [
        {
            "hooks": [{"type": "command", "command": hooks_dir + "/send-tool-status.sh"}]
        }
    ],
    "PostToolUseFailure": [
        {
            "hooks": [{"type": "command", "command": hooks_dir + "/send-status.sh ERROR"}]
        }
    ],
    "Stop": [
        {
            "hooks": [{"type": "command", "command": hooks_dir + "/send-status.sh DONE"}]
        }
    ]
}

settings = {}
if os.path.exists(settings_file):
    try:
        with open(settings_file) as f:
            settings = json.load(f)
    except (json.JSONDecodeError, IOError):
        pass

existing_hooks = settings.get("hooks", {})
for event, entries in clawy_hooks.items():
    if event not in existing_hooks:
        existing_hooks[event] = []
    existing_hooks[event] = [
        e for e in existing_hooks[event]
        if not any("/.clawy/" in h.get("command", "") for h in e.get("hooks", []))
    ]
    existing_hooks[event].extend(entries)

settings["hooks"] = existing_hooks

with open(settings_file, "w") as f:
    json.dump(settings, f, indent=2)
PYEOF

spin_step "Hooks added to settings.json"

# ── Step 3: Shell alias ────────────────────────────────────
echo -e "  ${C_CYAN}3${C_RESET} Shell alias"
echo ""

SHELL_PROFILE=""
if [ -n "$ZSH_VERSION" ] || [ "$SHELL" = "/bin/zsh" ]; then
  SHELL_PROFILE="$HOME/.zshrc"
elif [ -n "$BASH_VERSION" ] || [ "$SHELL" = "/bin/bash" ]; then
  SHELL_PROFILE="$HOME/.bashrc"
fi

if [ -n "$SHELL_PROFILE" ]; then
  if grep -q 'alias clawy=' "$SHELL_PROFILE" 2>/dev/null; then
    echo -e "   ${C_GREEN}Alias already in ${SHELL_PROFILE}${C_RESET}"
  else
    echo -e "   To launch Clawy sessions, you can add a shell alias:"
    echo -e "   ${C_DIM}alias clawy=\"CLAWY=1 claude\"${C_RESET}"
    echo ""
    read -p "   Add to $SHELL_PROFILE? [y/N] " REPLY
    if [[ "$REPLY" =~ ^[Yy]$ ]]; then
      echo '' >> "$SHELL_PROFILE"
      echo '# Clawy — Claude Code companion device' >> "$SHELL_PROFILE"
      echo 'alias clawy="CLAWY=1 claude"' >> "$SHELL_PROFILE"
      echo -e "   ${C_GREEN}Added!${C_RESET} Run ${C_DIM}source $SHELL_PROFILE${C_RESET} or open a new terminal."
    else
      echo -e "   ${C_DIM}Skipped — add it manually anytime.${C_RESET}"
    fi
  fi
fi

# ── Done ────────────────────────────────────────────────────
echo ""
echo -e "  ${C_DIM}────────────────────${C_RESET}"
echo ""

# Celebration animation
hide_cursor
clawy_happy; sleep 0.25; move_up 3
clawy_wave; sleep 0.35; move_up 3
clawy_happy; sleep 0.25; move_up 3
clawy_wave; sleep 0.35; move_up 3
clawy_happy
show_cursor

echo ""
echo -e "  ${C_GREEN}${C_BOLD}Installation complete!${C_RESET}"
echo ""
echo -e "  ${C_WHITE}Start a session:${C_RESET}"
echo -e "    ${C_CYAN}clawy${C_RESET}  ${C_DIM}(or CLAWY=1 claude)${C_RESET}"
echo ""
echo -e "  ${C_WHITE}Need to flash your device?${C_RESET}"
echo -e "    Visit the web flasher to install firmware"
echo -e "    and configure WiFi — right from your browser."
echo ""
echo -e "    ${C_CYAN}https://clawy.dev/flash${C_RESET}"
echo ""
