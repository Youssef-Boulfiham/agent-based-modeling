# Instruction Manuals

Final specification + artefact documents (.docx). Handoff reference for starting from scratch.

## Contents (Final deliverables)

- **Agent_Walking_Pathfinding_Manual.docx** — Walking & pathfinding behaviour (VETO — source of truth). Page 1: spec. Page 2+: artefact code (commit 32c49e0, 2026-06-18).
- **Programming_Agent_Workflow_Manual.docx** — Multi-stage pipeline: idea → deep search → approval → implementation → manual → artefact.
- **Textbox_Communication_Log_Manual.docx** — Message history with scrolling + text selection + clipboard copy. Inverted scroll (wheel > 0 = down/newer). Stable anchoring via seq ID (survives 100-entry trim). Two independent selection regions (history + input).

## Artefact rule — NEVER a separate file

Artefact is **embedded inside the manual `.docx`** (page 2+). No standalone files. Ever.

- Page 1 = textual specification.
- Page 2+ = verbatim code extraction (every function in execution order).
- Recovery: paste page 2+ into a clean project → exact approved outcome.

## How to regenerate

**Source scripts and build machinery are in the `generate/` subfolder.** Do not edit .docx directly.

```bash
cd generate/
npm install docx          # one-time
node build_textbox_manual.js     # → ../Textbox_Communication_Log_Manual.docx
node build_manual.js             # → ../Agent_Walking_Pathfinding_Manual.docx
node build_workflow_manual.js    # → ../Programming_Agent_Workflow_Manual.docx
```

After edits, regenerate .docx files and commit both source (.js) and output (.docx).
