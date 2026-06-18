const fs = require("fs");
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  AlignmentType, LevelFormat, HeadingLevel, BorderStyle, WidthType,
  ShadingType,
} = require("docx");

const ACCENT = "2D6CDF";
const cellBorder = { style: BorderStyle.SINGLE, size: 1, color: "CCCCCC" };
const borders = { top: cellBorder, bottom: cellBorder, left: cellBorder, right: cellBorder };
const cellMargins = { top: 60, bottom: 60, left: 120, right: 120 };

function headerCell(text, w) {
  return new TableCell({
    borders, width: { size: w, type: WidthType.DXA }, margins: cellMargins,
    shading: { fill: "D5E1F5", type: ShadingType.CLEAR },
    children: [new Paragraph({ children: [new TextRun({ text, bold: true, size: 18 })] })],
  });
}
function bodyCell(text, w, bold = false) {
  return new TableCell({
    borders, width: { size: w, type: WidthType.DXA }, margins: cellMargins,
    children: [new Paragraph({ children: [new TextRun({ text, bold, size: 18 })] })],
  });
}

const doc = new Document({
  styles: {
    default: { document: { run: { font: "Arial", size: 19 } } },
    paragraphStyles: [
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 30, bold: true, font: "Arial", color: "1A1A1A" },
        paragraph: { spacing: { before: 0, after: 80 }, outlineLevel: 0 } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 21, bold: true, font: "Arial", color: ACCENT },
        paragraph: { spacing: { before: 160, after: 60 }, outlineLevel: 1 } },
    ],
  },
  numbering: {
    config: [
      { reference: "steps",
        levels: [{ level: 0, format: LevelFormat.DECIMAL, text: "%1.", alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 460, hanging: 300 } } } }] },
      { reference: "bul",
        levels: [{ level: 0, format: LevelFormat.BULLET, text: "•", alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 460, hanging: 300 } } } }] },
    ],
  },
  sections: [{
    properties: {
      page: {
        size: { width: 11906, height: 16838 }, // A4
        margin: { top: 1080, right: 1080, bottom: 1080, left: 1080 },
      },
    },
    children: [
      new Paragraph({ heading: HeadingLevel.HEADING_1,
        children: [new TextRun("Agent Walking & Pathfinding — Instruction Manual")] }),
      new Paragraph({ spacing: { after: 120 }, children: [new TextRun({
        text: "Hoe een agent rationeel door de wereld beweegt. Dit één A4 beschrijft het volledige doel; geef het door om vanaf nul opnieuw te beginnen.",
        italics: true, size: 18, color: "555555" })] }),

      // 1. Doel
      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("1. Doel in één zin")] }),
      new Paragraph({ spacing: { after: 60 }, children: [new TextRun(
        "Een externe functie bepaalt in welk domein de agent moet zijn. Elke step checkt de agent dat: is hij er nog niet, dan loopt hij via de gang naar dat domein (kortste pad als priority-queue, één cel per stap); is hij er wel, dan loopt hij wat rond (idle) of staat hij stil (working).")] }),

      // 2. Kernconcepten
      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("2. Kernconcepten")] }),
      ...bulletRich([
        { text: "Domein stuurt het loopgedrag: ", bold: true },
        { text: "de ruimte waar de agent moet zijn. Wordt EXTERN gezet (andere module) en verandert zelden — minimaal elke 50 steps. De agent checkt elke step welk domein hij moet zijn; hij verandert dit zelf nooit." },
      ]),
      ...bulletRich([
        { text: "Gang verbindt domeinen: ", bold: true },
        { text: "naar een ander domein loopt de agent ALTIJD via de gang. Muren blokkeren; het pad is een kortste route (BFS/A*) over begaanbare cellen (kamers + gangen), nooit dwars door een muur." },
      ]),
      ...bulletRich([
        { text: "Activiteit (3 soorten): ", bold: true },
        { text: "idle = niks te doen, loopt wat rond in zijn kamer; working = blijft staan waar hij is; move to domain = nog onderweg via de gang naar het doel-domein." },
      ]),
      ...bullet("Pad = priority-queue: het hele pad van huidige cel naar doel wordt vooraf berekend als een wachtrij van cellen (front = volgende stap). De agent pakt elke step één cel van de voorkant."),

      // 3. Pathfinding regels
      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("3. Pathfinding — harde regels")] }),
      ...numbered("Eén stap per step: elke beweging gaat naar een buurcel. Chebyshev-afstand tot de vorige cel is altijd ≤ 1. De agent mag NOOIT springen van het ene punt op de map naar het andere."),
      ...numbered("Kortste route via de gang: het pad over begaanbare cellen is minimaal. Naar een ander domein loopt de agent door de gang, niet dwars door muren."),
      ...numbered("Vloeiend & volgbaar: opeenvolgende posities vormen een aaneengesloten lijn, zodat het gedrag voor een kijker rationeel en te volgen is."),
      ...numbered("Domein-check elke step: het doel-domein kan extern veranderd zijn. Zo ja, herplan en loop via de gang naar het nieuwe domein; zo nee, idle/working binnen het huidige domein."),

      // 4. Tick-loop
      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("4. De step-loop")] }),
      new Table({
        width: { size: 9746, type: WidthType.DXA },
        columnWidths: [2450, 7296],
        rows: [
          new TableRow({ tableHeader: true, children: [headerCell("Toestand", 2450), headerCell("Wat de agent doet", 7296)] }),
          new TableRow({ children: [bodyCell("check domein", 2450, true), bodyCell("Elke step: zit ik in het doel-domein? Het doel komt van buiten en is ≥ 50 steps stabiel.", 7296)] }),
          new TableRow({ children: [bodyCell("move to domain", 2450, true), bodyCell("Niet in doel-domein → plan/vervolg BFS-pad via de gang, loop één cel. Bij aankomst → idle of working.", 7296)] }),
          new TableRow({ children: [bodyCell("idle", 2450, true), bodyCell("In doel-domein, niks te doen → loop wat rond in de kamer (één cel per step).", 7296)] }),
          new TableRow({ children: [bodyCell("working", 2450, true), bodyCell("In doel-domein → blijf staan waar je bent (geen beweging).", 7296)] }),
        ],
      }),

      // 5. Acceptatiecriteria
      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("5. Klaar wanneer")] }),
      ...bullet("Elke beweging heeft stap-afstand 1 (nul “jumps” over de hele run)."),
      ...bullet("Reizen tussen domeinen lopen via de gang (kortste begaanbare route, niet door muren)."),
      ...bullet("Doel-domein wordt alleen extern gezet, is ≥ 50 steps stabiel, en wordt elke step gecheckt."),
      ...bullet("De drie activiteiten idle / working / move to domain gedragen zich correct."),

      new Paragraph({ spacing: { before: 140 }, border: { top: { style: BorderStyle.SINGLE, size: 6, color: ACCENT, space: 6 } },
        children: [new TextRun({ text: "Referentie-implementatie: testcases/html/walking_behaviour/index.html. Gevalideerd: 0 jumps, alle reizen via de gang, doel-domein ≥ 50 steps.",
          size: 17, italics: true, color: "555555" })] }),
    ],
  }],
});

function bullet(text) {
  return [new Paragraph({ numbering: { reference: "bul", level: 0 }, spacing: { after: 40 },
    children: [new TextRun({ text, size: 19 })] })];
}
function bulletRich(runs) {
  return [new Paragraph({ numbering: { reference: "bul", level: 0 }, spacing: { after: 40 },
    children: runs.map(r => new TextRun({ text: r.text, bold: !!r.bold, size: 19 })) })];
}
function numbered(text) {
  return [new Paragraph({ numbering: { reference: "steps", level: 0 }, spacing: { after: 40 },
    children: [new TextRun({ text, size: 19 })] })];
}

Packer.toBuffer(doc).then((buf) => {
  fs.writeFileSync("Agent_Walking_Pathfinding_Manual.docx", buf);
  console.log("written Agent_Walking_Pathfinding_Manual.docx");
});
