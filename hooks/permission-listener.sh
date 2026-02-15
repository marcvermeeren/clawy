#!/bin/bash
# PermissionRequest hook: approve/deny via physical buttons over WiFi
#
# Flow:
# 1. Claude needs permission -> this script runs
# 2. Extracts tool context from stdin JSON
# 3. Sends STATUS:APPROVE + MESSAGE:<context> to ESP32 (TCP port 7801)
# 4. Waits for BUTTON:APPROVE or BUTTON:DENY response
# 5. Returns JSON decision to Claude Code
# 6. 60s timeout -> falls through (returns empty, Claude shows terminal dialog)

# Only run when CLAWY=1 (set by `clawy` alias or manually)
[ "$CLAWY" != "1" ] && exit 0

TIMEOUT=60
CMD_PORT=7800
APPROVAL_PORT=7801

CACHE_FILE="/tmp/clawy-ip-$USER"
CACHE_TTL=3600  # 1 hour — covers most sessions

resolve_host() {
  if [ -f "$CACHE_FILE" ]; then
    if [[ "$(uname)" == "Darwin" ]]; then
      CACHE_AGE=$(( $(date +%s) - $(stat -f %m "$CACHE_FILE") ))
    else
      CACHE_AGE=$(( $(date +%s) - $(stat -c %Y "$CACHE_FILE") ))
    fi
    if [ "$CACHE_AGE" -lt "$CACHE_TTL" ]; then
      cat "$CACHE_FILE"
      return 0
    fi
  fi
  IP=$(python3 -c "import socket; print(socket.gethostbyname('clawy.local'))" 2>/dev/null)
  if [ -n "$IP" ]; then
    printf '%s' "$IP" > "$CACHE_FILE"
    echo "$IP"
    return 0
  fi
  echo "clawy.local"  # fallback if resolution fails
}

HOST=$(resolve_host)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Capture stdin (Claude Code sends request details as JSON)
STDIN_JSON=$(cat)

# Extract tool name — skip interactive tools that need terminal UI
TOOL_NAME=$(echo "$STDIN_JSON" | sed -n 's/.*"tool_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')

if [ "$TOOL_NAME" = "AskUserQuestion" ]; then
  # Extract first question text (python3 for reliable JSON parsing)
  QUESTION=$(echo "$STDIN_JSON" | python3 -c "
import sys, json
try:
    d = json.load(sys.stdin)
    qs = d.get('tool_input', {}).get('questions', [])
    print(qs[0]['question'] if qs else '')
except Exception:
    print('')
" 2>/dev/null)

  if [ -n "$QUESTION" ]; then
    "$SCRIPT_DIR/send-status.sh" INPUT "$QUESTION"
  else
    "$SCRIPT_DIR/send-status.sh" INPUT
  fi
  exit 0
fi

# Extract quest text from tool_name + tool_input for display context
QUEST_TEXT=""
case "$TOOL_NAME" in
  Bash)
    CMD=$(echo "$STDIN_JSON" | sed -n 's/.*"command"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -c 180)
    [ -n "$CMD" ] && QUEST_TEXT="Run: $CMD"
    ;;
  Edit)
    FP=$(echo "$STDIN_JSON" | sed -n 's/.*"file_path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')
    [ -n "$FP" ] && QUEST_TEXT="Edit ${FP##*/}"
    ;;
  Write)
    FP=$(echo "$STDIN_JSON" | sed -n 's/.*"file_path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')
    [ -n "$FP" ] && QUEST_TEXT="Write ${FP##*/}"
    ;;
  Read)
    FP=$(echo "$STDIN_JSON" | sed -n 's/.*"file_path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')
    [ -n "$FP" ] && QUEST_TEXT="Read ${FP##*/}"
    ;;
  Glob)
    PAT=$(echo "$STDIN_JSON" | sed -n 's/.*"pattern"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -c 80)
    [ -n "$PAT" ] && QUEST_TEXT="Search: $PAT"
    ;;
  Grep)
    PAT=$(echo "$STDIN_JSON" | sed -n 's/.*"pattern"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -c 80)
    [ -n "$PAT" ] && QUEST_TEXT="Search: $PAT"
    ;;
  Task)
    DESC=$(echo "$STDIN_JSON" | sed -n 's/.*"description"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -c 80)
    [ -n "$DESC" ] && QUEST_TEXT="Subagent: $DESC"
    ;;
  WebSearch|WebFetch)
    QRY=$(echo "$STDIN_JSON" | sed -n 's/.*"query"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -c 120)
    [ -n "$QRY" ] && QUEST_TEXT="Search: $QRY"
    ;;
  *)
    QUEST_TEXT="$TOOL_NAME"
    ;;
esac

# Fallback if extraction failed
[ -z "$QUEST_TEXT" ] && QUEST_TEXT="$TOOL_NAME"

# Serialize permission-listener instances via lock directory.
# Without this, rapid-fire permission requests (e.g. two WebSearches) race:
# hook #1 holds the TCP connection waiting for a button press, hook #2 can't
# acquire the lock, and Claude falls through to a terminal dialog.
LOCKDIR="/tmp/clawy-listener-$USER.lock"

# Clean stale lock (script crashed without releasing — older than TIMEOUT + 30s)
if [ -d "$LOCKDIR" ]; then
  if [[ "$(uname)" == "Darwin" ]]; then
    LOCK_AGE=$(( $(date +%s) - $(stat -f %m "$LOCKDIR") ))
  else
    LOCK_AGE=$(( $(date +%s) - $(stat -c %Y "$LOCKDIR") ))
  fi
  [ "$LOCK_AGE" -gt 90 ] && rmdir "$LOCKDIR" 2>/dev/null
fi

# Wait for another instance to finish (up to TIMEOUT + 5s)
LOCK_DEADLINE=$(( $(date +%s) + TIMEOUT + 5 ))
while ! mkdir "$LOCKDIR" 2>/dev/null; do
  [ "$(date +%s)" -ge "$LOCK_DEADLINE" ] && exit 0
  sleep 0.5
done
trap 'rmdir "$LOCKDIR" 2>/dev/null' EXIT

# Sanitize quest text (collapse newlines, trim)
QUEST_TEXT=$(printf '%s' "$QUEST_TEXT" | tr '\n' ' ' | head -c 180)

# Bidirectional TCP to approval port:
# Send STATUS:APPROVE + MESSAGE, keep socket open until button response or timeout.
# python3 avoids nc's half-close (printf EOF -> TCP FIN -> ESP32 drops client).
RESPONSE=$(HOST="$HOST" PORT="$APPROVAL_PORT" TIMEOUT="$TIMEOUT" \
  QUEST="$QUEST_TEXT" python3 << 'PYEOF'
import socket, os
host = os.environ['HOST']
port = int(os.environ['PORT'])
timeout = int(os.environ['TIMEOUT'])
quest = os.environ.get('QUEST', '')
try:
    s = socket.create_connection((host, port), timeout=2)
    s.settimeout(timeout)
    s.sendall(f'STATUS:APPROVE\nMESSAGE:{quest}\n'.encode())
    data = b''
    while b'\n' not in data:
        chunk = s.recv(256)
        if not chunk:
            break
        data += chunk
    s.close()
    print(data.split(b'\n')[0].decode().strip())
except Exception:
    pass
PYEOF
)

# Parse response
case "$RESPONSE" in
  BUTTON:APPROVE)
    echo '{"hookSpecificOutput":{"hookEventName":"PermissionRequest","decision":{"behavior":"allow"}}}'
    ;;
  BUTTON:DENY)
    echo '{"hookSpecificOutput":{"hookEventName":"PermissionRequest","decision":{"behavior":"deny"}}}'
    ;;
esac
