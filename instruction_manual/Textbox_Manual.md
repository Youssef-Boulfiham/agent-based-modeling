# Textbox Sandbox — Agent Communication Log

## Essence

Interactive message log for tracking agent-to-agent communication and user input. All messages persisted in browser localStorage. Clean single-line format with background color distinction (user vs agent). User input has highest priority in execution queue—agents pause, respond immediately, then resume work.

## Architecture

**Type:** HTML5 + JavaScript (client-side, no backend)  
**Location:** `testcases/html/textbox/index.html`  
**Storage:** localStorage key `textbox_history` (max 1000 entries, auto-trim)  
**Update Cycle:** 500ms agent tick, real-time user input via priority queue

## Core Data Structure

```javascript
{
  "timestamp": "2026-06-18T15:21:03.425Z",
  "from": "user" | "Agent A" | "Agent B" | "Agent C",
  "to": "Agent A" | "Agent B" | "Agent C",
  "text": "message content",
  "state": {
    "pos": { "x": number, "y": number },
    "domain": "Domain A" | "Domain B" | "Domain C" | "Domain D",
    "action": "idle" | "working" | "moving"
  }
}
```

State is captured at message time; stored for reference but not displayed in UI.

## Display Format

Single row per message (no line wraps):
```
15:21:03 user → Agent B: hello world
15:21:07 Agent A → Agent C: Moving to Domain D
```

Colors:
- User messages: dark blue background (`#1a2238`)
- Agent messages: dark green background (`#1a2a1a`)
- Text: light gray, monospace
- Timestamp: dim gray, smaller font

## UI Components

**Top section (always visible):**
- Frame counter (simulation tick)
- Agent count (always 3)
- Log entry count (total recorded)
- **"Hide Agent-Agent"** button: toggle inter-agent chatter visibility
- **"Jump to Bottom"** button: scroll to latest message (manual only, no auto-jump)

**Middle (main log area):**
- Scrollable div, contains all filtered log entries
- Height fills 80% of viewport
- Dark background, monospace font

**Bottom (input section):**
- Text input field: `placeholder="Type message... (Enter to send)"`
- Send button (blue)
- Input always enabled (never blocked by background work)

## Execution Model

### User Input Priority

1. User types in input field → triggers "Send"
2. Message added to `state.taskQueue` as **highest priority** (unshift)
3. Next tick (500ms):
   - `processTaskQueue()` runs first (before agent ticking)
   - User message extracted, logged, rendered
   - Agent chatter pauses (flag: `state.agentChatterPaused`)
4. User message persisted to localStorage
5. Render completes, agents resume work next tick

### Agent Ticking (500ms interval)

1. Check queue for user tasks (pause if found)
2. Update agent state: random action/domain changes (20% and 10% per agent)
3. Every ~20 frames, agents generate chatter to each other
4. Chatter added via `addLogEntry()`, persisted, rendered
5. Loop continues

### Log Rendering

Filter applied BEFORE render:
- If `state.showAgentAgent` is false: exclude agent-to-agent messages
- User messages always visible

Render loop:
- Clear container
- For each filtered entry:
  - Create div, set class ("log-entry user" or "log-entry agent")
  - Build HTML: `time from → to: text`
  - Append to container
- No auto-scroll (user controls scrolling)

## Persistence (localStorage)

**Key:** `textbox_history`  
**Format:** JSON stringified array of entry objects  
**Trigger:** After every `addLogEntry()`  
**Trim:** Max 1000 entries (oldest removed first)  
**Survive:** Browser reload (persists in localStorage)

## Replication Checklist

To rebuild from scratch:

1. **HTML structure:**
   - Single file, self-contained
   - Dark theme (`#14141a` background, `#e8e4dc` text)
   - Flex layout: title + controls + log-container + input

2. **CSS:**
   - `.log-entry.user`: dark blue background
   - `.log-entry.agent`: dark green background
   - Log font: 12px monospace, gray timestamp
   - Input field: 13px monospace, blue border on focus

3. **JavaScript state:**
   - 3 dummy agents (Agent A, B, C) with random pos/domain/action
   - 4 domains (A, B, C, D)
   - 3 actions (idle, working, moving)
   - Task queue for user input priority
   - Flag for agent chatter pause

4. **Main loop (requestAnimationFrame):**
   - Check elapsed time > 500ms since last tick
   - Call `tick()`: process queue, update state, render
   - Update lastTick time
   - Loop continues

5. **Storage:**
   - Load from localStorage on init
   - Save after every message (JSON.stringify)
   - Max 1000 entries, auto-trim oldest

6. **Buttons:**
   - Toggle Agent-Agent: flip `state.showAgentAgent`, re-render
   - Jump to Bottom: `container.scrollTop = container.scrollHeight`

7. **Input:**
   - Enter key or Send button → add to queue (unshift for priority)
   - Clear field after send
   - Always focus input after message

## Key Behaviors

- **No auto-scroll:** User controls scroll position. Manual "Jump to Bottom" only.
- **Priority queue:** User input interrupts agent work, processed first.
- **State capture:** Each message records agent state at time of sending (not displayed).
- **Filter toggle:** Hide/show agent-agent chatter without losing history.
- **Persistent:** All messages survive browser reload.
- **Single style:** No special colors per message type, only user vs agent distinction.

## Testing

Open in browser at `localhost:8733`. Type messages, watch agents respond. Toggle filter. Scroll through history. Refresh page — history persists.
