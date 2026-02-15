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

# Fire-and-forget: python3 socket (portable across macOS + Linux, no nc flags)
# Retry once with fresh mDNS resolve on failure
HOST="$HOST" PORT="$PORT" PAYLOAD="$PAYLOAD" CACHE_FILE="$CACHE_FILE" python3 -c "
import socket, os
def send(host, port, payload):
    s = socket.create_connection((host, port), timeout=1)
    s.sendall(payload.encode())
    s.close()
host = os.environ['HOST']
port = int(os.environ['PORT'])
payload = os.environ['PAYLOAD'].replace(r'\n', '\n')
try:
    send(host, port, payload)
except Exception:
    try:
        os.remove(os.environ['CACHE_FILE'])
    except OSError:
        pass
    try:
        host = socket.gethostbyname('clawy.local')
        with open(os.environ['CACHE_FILE'], 'w') as f:
            f.write(host)
        send(host, port, payload)
    except Exception:
        pass
" &
disown
exit 0
