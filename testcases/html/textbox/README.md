# Textbox Sandbox — Agent Communication Log

Interactive message log for tracking agent-to-agent communication and user input.

## Features

**Log display (scrollable)**
- Timestamp, sender, receiver, message
- Agent state: position (x,y), domain, action
- Filterable: toggle agent-agent messages on/off
- Persistent: localStorage keeps history between sessions

**Input row (bottom)**
- Type and send messages
- Cursor: arrow keys, mouse, selection all work
- Enter to send, clear after

**Dummy agents**
- 3 agents (A, B, C)
- Random state: action (idle/working/moving), position, domain
- Auto-chatter every ~20 frames (configurable)
- State visible in each log entry

**Controls**
- **Jump to Bottom**: scroll to latest message
- **Hide/Show Agent-Agent**: filter inter-agent chatter
- **Clear Log**: delete all history
- **Export as JSON**: download full log

## Architecture

- Single HTML/JS file (no backend)
- `state.log[]` array in localStorage (JSON lines format)
- Agents tick every 500ms
- Each entry: `{timestamp, from, to, text, state: {pos, domain, action}}`

## Testing

Open in browser. Type message → sent to random agent with state snapshot.
Watch agents chatter to each other. Toggle filter. Export to see JSON format.

Max 1000 entries (auto-trimmed on save).
