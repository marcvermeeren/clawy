#!/bin/bash
# Clawy OpenClaw installer — deploys an OpenClaw internal hook for status mirroring
set -e

HOOK_NAME="clawy-status"
HOOKS_ROOT="$HOME/.openclaw/hooks"
DEST_DIR="$HOOKS_ROOT/$HOOK_NAME"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR/openclaw-hooks/$HOOK_NAME"

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
echo -e "  ${C_MAGENTA}C${C_YELLOW}l${C_WHITE}a${C_PINK}w${C_GREEN}y${C_RESET} ${C_DIM}OpenClaw installer${C_RESET}"
echo -e "  ${C_DIM}────────────────────────────${C_RESET}"
echo ""

if ! command -v openclaw >/dev/null 2>&1; then
  echo -e "  ${C_PINK}openclaw is not installed or not on PATH.${C_RESET}"
  exit 1
fi

if [ ! -d "$SOURCE_DIR" ]; then
  echo -e "  ${C_PINK}Hook source not found:${C_RESET} $SOURCE_DIR"
  exit 1
fi

mkdir -p "$HOOKS_ROOT"
rm -rf "$DEST_DIR"
cp -R "$SOURCE_DIR" "$DEST_DIR"

echo -e "  ${C_CYAN}1${C_RESET} Installed hook files to ${C_DIM}$DEST_DIR${C_RESET}"

openclaw hooks enable "$HOOK_NAME"
echo -e "  ${C_CYAN}2${C_RESET} Enabled hook ${C_DIM}$HOOK_NAME${C_RESET}"

openclaw gateway restart
echo -e "  ${C_CYAN}3${C_RESET} Restarted OpenClaw Gateway"

echo ""
echo -e "  ${C_GREEN}${C_BOLD}Installation complete!${C_RESET}"
echo ""
echo -e "  ${C_WHITE}Current OpenClaw mapping:${C_RESET}"
echo -e "    ${C_CYAN}READY${C_RESET}    startup / new / reset"
echo -e "    ${C_CYAN}WORKING${C_RESET}  inbound message"
echo -e "    ${C_CYAN}DONE${C_RESET}     successful reply"
echo -e "    ${C_CYAN}INPUT${C_RESET}    likely question reply (best effort heuristic)"
echo -e "    ${C_CYAN}ERROR${C_RESET}    failed outbound send"
echo ""
echo -e "  ${C_DIM}Tool-by-tool status and approve/deny parity still need a native${C_RESET}"
echo -e "  ${C_DIM}OpenClaw plugin/runtime-hook integration.${C_RESET}"
echo ""
