---
name: clawy-status
description: "Mirror OpenClaw session status to a Clawy device over LAN TCP"
metadata:
  {
    "openclaw":
      {
        "emoji": "🐾",
        "events": [
          "gateway:startup",
          "agent:bootstrap",
          "command:new",
          "command:reset",
          "command:stop",
          "message:received",
          "message:sent"
        ]
      }
  }
---

# Clawy OpenClaw Status Hook

Bridges OpenClaw lifecycle and message events to the Clawy device protocol.

## What it maps

- `gateway:startup` / `agent:bootstrap` / `command:new` / `command:reset` -> `READY`
- `message:received` -> `WORKING`
- `message:sent` -> `DONE`, `INPUT`, or `ERROR`
- `command:stop` -> `DONE`

## Current limitations

This hook uses OpenClaw internal hooks, so it covers session/message status well,
but it does **not** yet reproduce full Claude Code parity for:

- per-tool `TOOL:<label>` states
- physical approve/deny of OpenClaw approval prompts

Those require an OpenClaw plugin/runtime-hook integration rather than internal
Gateway event hooks alone.
