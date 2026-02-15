#!/bin/bash
# Clawy uninstaller — removes hooks and ~/.clawy directory
set -e

CLAWY_DIR="$HOME/.clawy"
SETTINGS_FILE="$HOME/.claude/settings.json"

echo "=== Clawy Uninstaller ==="
echo ""

# Step 1: Remove Clawy hooks from settings.json
if [ -f "$SETTINGS_FILE" ]; then
  echo "1. Removing hooks from $SETTINGS_FILE..."
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
    # Remove empty arrays
    if not hooks[event]:
        del hooks[event]

if not hooks:
    del settings["hooks"]

if changed:
    with open(settings_file, "w") as f:
        json.dump(settings, f, indent=2)
    print("   Hooks removed.")
else:
    print("   No Clawy hooks found.")
PYEOF
else
  echo "1. No settings file found, skipping."
fi

# Step 2: Remove ~/.clawy directory
if [ -d "$CLAWY_DIR" ]; then
  echo "2. Removing $CLAWY_DIR..."
  rm -rf "$CLAWY_DIR"
  echo "   Done."
else
  echo "2. $CLAWY_DIR not found, skipping."
fi

# Step 3: Clean up temp files
echo "3. Cleaning up temp files..."
rm -f "/tmp/clawy-ip-$USER"
rmdir "/tmp/clawy-listener-$USER.lock" 2>/dev/null || true
echo "   Done."

echo ""
echo "=== Uninstall complete ==="
echo ""
echo "Note: If you added 'alias clawy=...' to your shell profile,"
echo "you'll need to remove that line manually."
echo ""
