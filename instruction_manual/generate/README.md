# Manual Generators

Build machinery for creating instruction manual .docx files. Source scripts only — **do not commit generated .docx here.**

## Contents

- **build_manual.js** — Walking Pathfinding Manual generator.
- **build_textbox_manual.js** — Textbox Communication Log Manual generator (scrolling + selection + copy).
- **build_workflow_manual.js** — Programming Agent Workflow Manual generator.
- **package.json** + **node_modules/** — docx library + dependencies.

## Workflow

1. **Edit source.** Update `.js` file with spec changes or new artefact code.
2. **Regenerate.** Run `node build_XXX_manual.js` to output .docx.
3. **Commit both.** Stage the source .js file AND the generated .docx in the parent folder (`instruction_manual/*.docx`).
4. **Never edit .docx directly.** Always edit the source .js and regenerate.

## Setup (one-time)

```bash
npm install
```

## Generate

```bash
node build_textbox_manual.js     # → ../Textbox_Communication_Log_Manual.docx
node build_manual.js             # → ../Agent_Walking_Pathfinding_Manual.docx
node build_workflow_manual.js    # → ../Programming_Agent_Workflow_Manual.docx
```

## artefactSection() / artefactNote() / code()

Helper functions (defined in each .js file) format the artefact:

- `artefactSection(num, title)` — numbered section header with blue/gray styling.
- `artefactNote(text)` — subsection label (e.g., "4a. processQueue()").
- `code(lines)` — monospace code block with dark background + syntax highlighting.

Use these to keep artefact formatting consistent across all manuals.
