#!/bin/bash
# Clawy OpenClaw uninstaller — removes the OpenClaw hook deployment
set -e

HOOK_NAME="clawy-status"
DEST_DIR="$HOME/.openclaw/hooks/$HOOK_NAME"

C_MAGENTA='\033[95m'
C_YELLOW='\033[93m'
C_WHITE='\033[97m'
C_PINK='\033[35m'
C_GREEN='\033[92m'
C_CYAN='\033[96m'
C_DIM='\033[2m'
C_BOLD='\033[1m'
C_RESET='\033[0m'

echo ""
echo -e "  ${C_MAGENTA}C${C_YELLOW}l${C_WHITE}a${C_PINK}w${C_GREEN}y${C_RESET} ${C_DIM}OpenClaw uninstaller${C_RESET}"
echo -e "  ${C_DIM}──────────────────────────────${C_RESET}"
echo ""

if ! command -v openclaw >/dev/null 2>&1; then
  echo -e "  ${C_PINK}openclaw is not installed or not on PATH.${C_RESET}"
  exit 1
fi

openclaw hooks disable "$HOOK_NAME" >/dev/null 2>&1 || true
echo -e "  ${C_CYAN}1${C_RESET} Disabled hook ${C_DIM}$HOOK_NAME${C_RESET}"

rm -rf "$DEST_DIR"
echo -e "  ${C_CYAN}2${C_RESET} Removed ${C_DIM}$DEST_DIR${C_RESET}"

openclaw gateway restart
echo -e "  ${C_CYAN}3${C_RESET} Restarted OpenClaw Gateway"

echo ""
echo -e "  ${C_GREEN}${C_BOLD}Uninstall complete!${C_RESET}"
echo ""
