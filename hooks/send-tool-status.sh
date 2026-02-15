#!/bin/bash
# PreToolUse hook: maps Claude tool names to friendly display labels
# Reads JSON from stdin, extracts tool_name, sends TOOL:<label> to device

INPUT=$(cat)

# Extract tool_name from JSON
TOOL=$(echo "$INPUT" | sed -n 's/.*"tool_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')
[ -z "$TOOL" ] && exit 0

# Map tool names to display labels
case "$TOOL" in
  Read)                    LABEL="Reading" ;;
  Edit)                    LABEL="Editing" ;;
  Write)                   LABEL="Writing" ;;
  Bash)                    LABEL="Running" ;;
  Grep|Glob)               LABEL="Searching" ;;
  Task)                    LABEL="Delegating" ;;
  WebFetch|WebSearch)      LABEL="Browsing" ;;
  NotebookEdit)            LABEL="Notebook" ;;
  AskUserQuestion)         LABEL="Asking" ;;
  *)                       LABEL="Working" ;;
esac

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
"$SCRIPT_DIR/send-status.sh" "TOOL:$LABEL"
