#!/bin/bash
# Fire-and-forget status update to Clawy over WiFi (mDNS -> TCP port 7800)
# Usage: ./send-status.sh <STATUS> [MESSAGE]
# Example: ./send-status.sh DONE
#          ./send-status.sh "TOOL:Reading"
#          ./send-status.sh APPROVE "Allow Edit tool on /src/main.rs?"

# Only run when CLAWY=1 (set by `clawy` alias or manually)
[ "$CLAWY" != "1" ] && exit 0

STATUS="$1"
MESSAGE="$2"
[ -z "$STATUS" ] && exit 0

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
PORT=7800

# Build payload
PAYLOAD="STATUS:$STATUS\n"
if [ -n "$MESSAGE" ]; then
  PAYLOAD="${PAYLOAD}MESSAGE:$MESSAGE\n"
fi

# Fire-and-forget: send via netcat, background + disown
# -G 1 = 1s connect timeout, -w 1 = 1s idle timeout
# Retry once with fresh mDNS resolve on failure
(
  printf "$PAYLOAD" | nc -G 1 -w 1 "$HOST" "$PORT" 2>/dev/null
  if [ $? -ne 0 ]; then
    rm -f "$CACHE_FILE"
    HOST=$(python3 -c "import socket; print(socket.gethostbyname('clawy.local'))" 2>/dev/null)
    [ -n "$HOST" ] && printf '%s' "$HOST" > "$CACHE_FILE" && printf "$PAYLOAD" | nc -G 1 -w 1 "$HOST" "$PORT" 2>/dev/null
  fi
) &
disown
exit 0
