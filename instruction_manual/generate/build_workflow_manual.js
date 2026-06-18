const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  Footer, AlignmentType, BorderStyle, WidthType, ShadingType,
  PageBreak
} = require("docx");
const fs = require("fs");

const DARK_BLUE = "1F3864";
const MID_BLUE  = "2E75B6";
const BLACK     = "000000";

function rule() {
  return new Paragraph({
    border: { bottom: { style: BorderStyle.SINGLE, size: 4, color: MID_BLUE, space: 1 } },
    spacing: { before: 0, after: 160 },
    children: []
  });
}

function stageRow(num, label, description) {
  const numBorder = { style: BorderStyle.NONE };
  const noBorder  = { top: numBorder, bottom: numBorder, left: numBorder, right: numBorder };
  return new TableRow({
    children: [
      // Stage number pill
      new TableCell({
        borders: noBorder,
        width: { size: 600, type: WidthType.DXA },
        shading: { fill: MID_BLUE, type: ShadingType.CLEAR },
        margins: { top: 60, bottom: 60, left: 80, right: 80 },
        verticalAlign: "center",
        children: [new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { before: 0, after: 0 },
          children: [new TextRun({ text: num, bold: true, size: 18, font: "Arial", color: "FFFFFF" })]
        })]
      }),
      // Stage label
      new TableCell({
        borders: noBorder,
        width: { size: 1700, type: WidthType.DXA },
        shading: { fill: "E9EFF7", type: ShadingType.CLEAR },
        margins: { top: 60, bottom: 60, left: 120, right: 80 },
        children: [new Paragraph({
          spacing: { before: 0, after: 0 },
          children: [new TextRun({ text: label, bold: true, size: 18, font: "Arial", color: DARK_BLUE })]
        })]
      }),
      // Description
      new TableCell({
        borders: noBorder,
        width: { size: 6726, type: WidthType.DXA },
        shading: { fill: "F7F9FC", type: ShadingType.CLEAR },
        margins: { top: 60, bottom: 60, left: 120, right: 80 },
        children: [new Paragraph({
          spacing: { before: 0, after: 0 },
          children: [new TextRun({ text: description, size: 18, font: "Arial", color: BLACK })]
        })]
      }),
    ]
  });
}

function spacerRow() {
  const noBorder = { style: BorderStyle.NONE };
  const nb = { top: noBorder, bottom: noBorder, left: noBorder, right: noBorder };
  return new TableRow({
    children: [
      new TableCell({ borders: nb, width: { size: 600,  type: WidthType.DXA }, children: [new Paragraph({ spacing: { before: 0, after: 0 }, children: [] })] }),
      new TableCell({ borders: nb, width: { size: 1700, type: WidthType.DXA }, children: [new Paragraph({ spacing: { before: 0, after: 0 }, children: [] })] }),
      new TableCell({ borders: nb, width: { size: 6726, type: WidthType.DXA }, children: [new Paragraph({ spacing: { before: 0, after: 0 }, children: [] })] }),
    ]
  });
}

function codeBlock(lines) {
  const nb = { style: BorderStyle.NONE };
  const noBorder = { top: nb, bottom: nb, left: nb, right: nb };
  const rows = lines.map(line =>
    new TableRow({
      children: [new TableCell({
        borders: noBorder,
        width: { size: 9026, type: WidthType.DXA },
        shading: { fill: "1E1E1E", type: ShadingType.CLEAR },
        margins: { top: 30, bottom: 30, left: 160, right: 160 },
        children: [new Paragraph({
          spacing: { before: 0, after: 0 },
          children: [new TextRun({ text: line, font: "Courier New", size: 16, color: "D4D4D4" })]
        })]
      })]
    })
  );
  const leftAccent = { style: BorderStyle.SINGLE, size: 8, color: MID_BLUE };
  const subtle     = { style: BorderStyle.SINGLE, size: 2, color: "555555" };
  return new Table({
    width: { size: 9026, type: WidthType.DXA },
    columnWidths: [9026],
    borders: { top: subtle, bottom: subtle, left: leftAccent, right: subtle,
               insideH: { style: BorderStyle.NONE }, insideV: { style: BorderStyle.NONE } },
    rows
  });
}

const footer = new Footer({
  children: [new Paragraph({
    alignment: AlignmentType.CENTER,
    children: [new TextRun({
      text: "ABM Project  ·  Programming Agent Workflow Manual  ·  v1.0",
      size: 16, color: "888888", font: "Arial"
    })]
  })]
});

const stages = [
  ["01", "Idea",          "User brings an idea. Agent registers it and gives a short response: confirm what holds up, flag what needs scrutiny. No deep work yet."],
  ["02", "Deep Search",   "Agent searches the full ABM codebase. Extracts relevant logic, data structures, and constraints. Generates new angles the user may not have considered."],
  ["03", "Report",        "Agent delivers a second response with findings: what was found, what conflicts with the idea, what new directions are possible. No code yet."],
  ["04", "Approval",      "User approves execution. Scope is agreed. Only then does implementation begin."],
  ["05", "Agent Work",    "Agent uses a large language model to work out the full implementation plan and code. Draws on everything discussed."],
  ["06", "Keep Updated",  "Agent notifies user of anything found during implementation that the user would likely want to know. No silence on surprises."],
  ["07", "Testcases",     "Implementation is written first in testcases/ (HTML sandbox or cpp testcase). Not in the main ABM yet."],
  ["08", "Iteration",     "Agent and user refine together until behaviour matches the idea. Testcase is the proving ground."],
  ["09", "Push",          "User instructs push to main. Agent pushes. Never automatic."],
  ["10", "Write Manual",  "Agent writes or updates the instruction manual (.docx) for the implemented behaviour. Stored in instruction_manual/."],
  ["11", "Implement ABM", "Agent integrates the approved testcase behaviour into the main ABM (src/). Follows the manual exactly."],
  ["12", "Log Version",   "Agent writes the artefact: verbatim code extraction of all functions controlling the behaviour, in execution order. Stored as instruction_manual/<Name>_Artefact_v<N>.md."],
  ["13", "Build Final Product", "Rebuild the root binary so it is always the newest build: cmake --build build && cp build/AgentBasedModeling ./AgentBasedModeling. AgentBasedModeling.app (double-click) launches that fresh root binary with no rebuild on launch -- it is the final product the user opens to check the latest version. testcases/ are only sandboxes. Always rebuild into root after every change."],
];

const doc = new Document({
  sections: [
    // PAGE 1 — Workflow Pipeline
    {
      properties: {
        page: {
          size: { width: 11906, height: 16838 },
          margin: { top: 1000, right: 1000, bottom: 1000, left: 1000 }
        }
      },
      footers: { default: footer },
      children: [
        new Paragraph({
          spacing: { before: 0, after: 40 },
          children: [new TextRun({ text: "Programming Agent — Workflow Manual", bold: true, size: 36, font: "Arial", color: DARK_BLUE })]
        }),
        new Paragraph({
          spacing: { before: 0, after: 200 },
          children: [new TextRun({ text: "Full pipeline: idea to final product  ·  Version 1.1", size: 20, font: "Arial", color: "666666" })]
        }),
        rule(),
        new Paragraph({
          spacing: { before: 0, after: 160 },
          children: [new TextRun({ text: "This manual defines how a programming agent (Claude Code) handles every idea from first mention to production. The pipeline is linear. No stage is skipped. Implementation never starts before approval.", size: 19, font: "Arial", color: BLACK })]
        }),

        // Pipeline table
        new Table({
          width: { size: 9026, type: WidthType.DXA },
          columnWidths: [600, 1700, 6726],
          borders: {
            top: { style: BorderStyle.NONE }, bottom: { style: BorderStyle.NONE },
            left: { style: BorderStyle.NONE }, right: { style: BorderStyle.NONE },
            insideH: { style: BorderStyle.SINGLE, size: 2, color: "DDDDDD" },
            insideV: { style: BorderStyle.NONE }
          },
          rows: stages.flatMap((s, i) => i < stages.length - 1
            ? [stageRow(s[0], s[1], s[2]), spacerRow()]
            : [stageRow(s[0], s[1], s[2])]
          )
        }),

        new Paragraph({ spacing: { before: 200, after: 0 }, children: [] }),
        rule(),
        new Paragraph({
          spacing: { before: 80, after: 0 },
          children: [new TextRun({ text: "Artefact template on the next page. Testcases live in testcases/. Manuals live in instruction_manual/.", size: 17, font: "Arial", color: "666666", italics: true })]
        }),

        new Paragraph({ children: [new PageBreak()] }),
      ]
    },

    // PAGE 2 — Artefact Template
    {
      properties: {
        page: {
          size: { width: 11906, height: 16838 },
          margin: { top: 1000, right: 1000, bottom: 1000, left: 1000 }
        }
      },
      footers: { default: footer },
      children: [
        new Paragraph({
          spacing: { before: 0, after: 40 },
          children: [new TextRun({ text: "Attachment — Artefact Template", bold: true, size: 32, font: "Arial", color: DARK_BLUE })]
        }),
        new Paragraph({
          spacing: { before: 0, after: 200 },
          children: [new TextRun({ text: "Stage 12 deliverable. Copy this structure for every new artefact.", size: 18, font: "Arial", color: "666666", italics: true })]
        }),
        rule(),

        new Paragraph({ spacing: { before: 0, after: 80 }, children: [new TextRun({ text: "File header", bold: true, size: 20, font: "Arial", color: DARK_BLUE })] }),
        codeBlock([
          "# [Behaviour Name] Artefact -- v[N]",
          "## Source: instruction_manual/[ManualName].docx",
          "## Commit: [git commit hash] -- [YYYY-MM-DD]",
          "## Branch: main",
        ]),
        new Paragraph({ spacing: { before: 0, after: 140 }, children: [] }),

        new Paragraph({ spacing: { before: 0, after: 80 }, children: [new TextRun({ text: "1.  Env — Relevant Fields", bold: true, size: 20, font: "Arial", color: DARK_BLUE })] }),
        codeBlock([
          "// Verbatim field declarations from Env.h that the behaviour reads or writes.",
          "std::vector<Agent> agents;",
          "std::vector<Domain> domains;",
          "int gridW, gridH;",
          "// ...",
        ]),
        new Paragraph({ spacing: { before: 0, after: 140 }, children: [] }),

        new Paragraph({ spacing: { before: 0, after: 80 }, children: [new TextRun({ text: "2.  Simulation Loop  (System.cpp or main.cpp)", bold: true, size: 20, font: "Arial", color: DARK_BLUE })] }),
        codeBlock([
          "// Verbatim while-loop that drives the simulation.",
          "while (running) {",
          "    env.step();",
          "    // ...",
          "}",
        ]),
        new Paragraph({ spacing: { before: 0, after: 140 }, children: [] }),

        new Paragraph({ spacing: { before: 0, after: 80 }, children: [new TextRun({ text: "3.  Env::step() — Frame Stepper", bold: true, size: 20, font: "Arial", color: DARK_BLUE })] }),
        codeBlock([
          "// Verbatim Env::step()  --  src/Env.cpp",
          "void Env::step() {",
          "    for (auto& agent : agents) {",
          "        agent.step(*this);",
          "    }",
          "}",
        ]),
        new Paragraph({ spacing: { before: 0, after: 140 }, children: [] }),

        new Paragraph({ spacing: { before: 0, after: 80 }, children: [new TextRun({ text: "4.  Agent::step() — Agent Logic", bold: true, size: 20, font: "Arial", color: DARK_BLUE })] }),
        codeBlock([
          "// Verbatim Agent::step()  --  src/Agent.cpp",
          "void Agent::step(Env& env) {",
          "    // ...",
          "}",
        ]),
        new Paragraph({ spacing: { before: 0, after: 140 }, children: [] }),

        new Paragraph({ spacing: { before: 0, after: 80 }, children: [new TextRun({ text: "5.  Sub-functions  (in call order)", bold: true, size: 20, font: "Arial", color: DARK_BLUE })] }),
        codeBlock([
          "// Every helper function called by the behaviour, verbatim, in call order.",
          "",
          "// Function 1: [name]  --  src/[File].cpp",
          "ReturnType ClassName::functionName(...) {",
          "    // verbatim body",
          "}",
          "",
          "// Function 2: [name]",
          "// ...",
        ]),
        new Paragraph({ spacing: { before: 0, after: 180 }, children: [] }),

        rule(),
        new Paragraph({
          spacing: { before: 80, after: 0 },
          children: [new TextRun({ text: "End of artefact. Pasting sections 1–5 into a clean project reproduces the approved behaviour exactly.", size: 17, font: "Arial", color: "555555", italics: true })]
        }),
      ]
    }
  ]
});

Packer.toBuffer(doc).then(buf => {
  fs.writeFileSync(
    "/Users/youssefboulfiham/agent-based_modeling/instruction_manual/Programming_Agent_Workflow_Manual.docx",
    buf
  );
  console.log("Written: Programming_Agent_Workflow_Manual.docx");
});
