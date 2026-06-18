const fs = require("fs");
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  AlignmentType, LevelFormat, HeadingLevel, BorderStyle, WidthType,
  ShadingType, PageBreak,
} = require("docx");

const ACCENT = "2D6CDF";

function artefactSection(num, title) {
  const nb = { style: BorderStyle.NONE };
  const noBorder = { top: nb, bottom: nb, left: nb, right: nb };
  return [
    new Paragraph({ spacing: { before: 200, after: 0 }, children: [] }),
    new Table({
      width: { size: 9746, type: WidthType.DXA },
      columnWidths: [560, 9186],
      borders: { top: nb, bottom: nb, left: nb, right: nb, insideH: nb, insideV: nb },
      rows: [new TableRow({ children: [
        new TableCell({
          borders: noBorder, width: { size: 560, type: WidthType.DXA },
          shading: { fill: "2E75B6", type: ShadingType.CLEAR },
          margins: { top: 60, bottom: 60, left: 80, right: 80 },
          children: [new Paragraph({ alignment: AlignmentType.CENTER, spacing: { before: 0, after: 0 },
            children: [new TextRun({ text: num, bold: true, size: 18, font: "Arial", color: "FFFFFF" })] })]
        }),
        new TableCell({
          borders: noBorder, width: { size: 9186, type: WidthType.DXA },
          shading: { fill: "EEF2FA", type: ShadingType.CLEAR },
          margins: { top: 60, bottom: 60, left: 140, right: 80 },
          children: [new Paragraph({ spacing: { before: 0, after: 0 },
            children: [new TextRun({ text: title, bold: true, size: 19, font: "Arial", color: "1F3864" })] })]
        }),
      ]})]
    }),
    new Paragraph({ spacing: { before: 0, after: 80 }, children: [] }),
  ];
}

function artefactNote(text) {
  return [new Paragraph({ spacing: { before: 120, after: 40 },
    children: [new TextRun({ text, bold: true, size: 17, font: "Arial", color: "2E75B6" })] })];
}

function code(lines) {
  const nb = { style: BorderStyle.NONE };
  const noBorder = { top: nb, bottom: nb, left: nb, right: nb };
  const rows = lines.map(line =>
    new TableRow({ children: [new TableCell({
      borders: noBorder, width: { size: 9746, type: WidthType.DXA },
      shading: { fill: "1A1A2E", type: ShadingType.CLEAR },
      margins: { top: 18, bottom: 18, left: 160, right: 80 },
      children: [new Paragraph({ spacing: { before: 0, after: 0 },
        children: [new TextRun({ text: line.length ? line : " ", font: "Courier New", size: 15, color: "E8E8E8" })] })]
    })] })
  );
  return [new Table({
    width: { size: 9746, type: WidthType.DXA }, columnWidths: [9746],
    borders: {
      top:    { style: BorderStyle.SINGLE, size: 2, color: "333355" },
      bottom: { style: BorderStyle.SINGLE, size: 2, color: "333355" },
      left:   { style: BorderStyle.SINGLE, size: 8, color: "2E75B6" },
      right:  { style: BorderStyle.SINGLE, size: 2, color: "333355" },
      insideH: nb, insideV: nb,
    },
    rows,
  }), new Paragraph({ spacing: { before: 0, after: 60 }, children: [] })];
}

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
        children: [new TextRun("Textbox — Agent Communication Log — Instruction Manual")] }),
      new Paragraph({ spacing: { after: 120 }, children: [new TextRun({
        text: "Message history with scrolling, text selection (drag/double/triple-click), and clipboard copy. Inverted scroll direction, stable scroll anchoring via seq ID, independent selection regions (history + input). This A4 describes the full goal; hand it over to start again from scratch.",
        italics: true, size: 18, color: "555555" })] }),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("1. Goal in one sentence")] }),
      new Paragraph({ spacing: { after: 60 }, children: [new TextRun(
        "Every message is recorded in a persistent, scrollable history with inverted scroll direction, full text selection in two independent regions (history + input), and clipboard copy via Cmd+C / Ctrl+C. Scroll anchors on stable per-entry IDs so selections survive message trim.")] }),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("2. Core concepts")] }),
      ...bulletRich([
        { text: "Priority queue: ", bold: true },
        { text: "all messages enter a queue. User input jumps to the FRONT (highest priority); agent chatter appends to the back. One task per frame." },
      ]),
      ...bulletRich([
        { text: "Message record: ", bold: true },
        { text: "each entry stores timestamp, from, to, text, sender state (pos, domain, action), and stable monotonic seq ID." },
      ]),
      ...bulletRich([
        { text: "Persistence & cap: ", bold: true },
        { text: "every entry appended to data/logs/textbox_history.jsonl (permanent). History reloads on restart. In-memory buffer CAPPED AT 100 ENTRIES — oldest messages trimmed to stay under limit. Seq ID survives trim so scroll anchors never break." },
      ]),
      ...bulletRich([
        { text: "Scroll direction: ", bold: true },
        { text: "mouse wheel / trackpad > 0 moves DOWN (toward newer). PgUp/PgDn reversed. Can scroll all the way down until only the last message remains." },
      ]),
      ...bulletRich([
        { text: "Scroll anchoring: ", bold: true },
        { text: "history rows anchored by per-entry seq, not position index. Survives log front-trim — anchored message never drifts." },
      ]),
      ...bulletRich([
        { text: "Text selection: ", bold: true },
        { text: "drag = select characters/rows. Double-click = word. Triple-click = whole row/line. Two independent regions: history (multiline) and input (single line)." },
      ]),
      ...bulletRich([
        { text: "Copy to clipboard: ", bold: true },
        { text: "Cmd+C (Mac) or Ctrl+C copies active selection. History selections joined by newline; input is plain text." },
      ]),
      ...bullet("Filter toggle: agent-to-agent chatter hidden/shown without losing history. User messages always visible."),
      ...bullet("Scrollbar: visual indicator on right edge. Thumb height = content fraction. Color = state: green (live/pinned), amber (manually scrolled)."),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("3. Display — hard rules")] }),
      ...numbered("One row per message: \"HH:MM:SS  from → to: text\". No wrapping."),
      ...numbered("Background color: user messages dark blue, agent messages dark green."),
      ...numbered("Scroll inverted: wheel > 0 / PgDn = down (newer). wheel < 0 / PgUp = up (older). Can scroll past full page down to last message only."),
      ...numbered("Text selection highlighted blue. Two regions (history + input) independent. Drag / double / triple work in both."),
      ...numbered("Scrollbar on right: green = live (pinned), amber = scrolled (manually positioned). Thumb height = fraction of content."),
      ...numbered("Two buttons only: \"Hide/Show Agent-Agent\" and \"Jump to Bottom\". Overlay on top-right, float in front of first message row."),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("4. Done when")] }),
      ...bullet("Scroll inverted: mouse wheel > 0 / PgDn moves down (newer). wheel < 0 / PgUp moves up (older)."),
      ...bullet("Can scroll past the full page down until only the last message is visible."),
      ...bullet("Message history stable during scroll: uses seq ID, not index (survives log trim)."),
      ...bullet("Drag, double-click (word), triple-click (row) all work in both regions (history + input)."),
      ...bullet("Cmd+C / Ctrl+C copies selection to system clipboard. History rows joined by newline."),
      ...bullet("Scrollbar shows position and state (green = live, amber = scrolled)."),

      new Paragraph({ spacing: { before: 140 }, border: { top: { style: BorderStyle.SINGLE, size: 6, color: ACCENT, space: 6 } },
        children: [new TextRun({ text: "Implementation: src/ChatBox.cpp (render + event handling), src/include/ChatBox.h (selection state), src/System.cpp (event wiring).",
          size: 17, italics: true, color: "555555" })] }),

      // ── ARTEFACT ────────────────────────────────────────────────────────────
      new Paragraph({ children: [new PageBreak()] }),

      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Artefact v1 — Verbatim Code", bold: true, size: 32, font: "Arial", color: "1F3864" })] }),
      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Commit: (latest dev)  —  2026-06-18  —  Branch: dev", size: 17, font: "Courier New", color: "555555" })] }),
      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Execution: (Env wires events) → System::handleEvents → ChatBox event handlers → render → scroll resolution. Paste sections 1–6 to reproduce the approved ChatBox scrolling and selection behaviour exactly.",
        size: 17, italics: true, color: "555555" })] }),
      new Paragraph({ border: { bottom: { style: BorderStyle.SINGLE, size: 4, color: "2E75B6", space: 1 } }, spacing: { before: 0, after: 180 }, children: [] }),

      ...artefactSection("1", "ChatBox Header — Fields & Selection State (src/include/ChatBox.h)"),
      ...code([
        "class ChatBox {",
        "private:",
        "    // ── Scroll model ────────────────────────────────────────────────",
        "    bool pinnedToBottom = true;",
        "    unsigned long anchorSeq = 0;    // stable per-entry id",
        "    int lastMaxTop = 0;",
        "    float scrollAccum = 0.0f;",
        "    int pendingScroll = 0;",
        "",
        "    // ── Text selection state ────────────────────────────────────────",
        "    struct VisRow { unsigned long seq; int y; std::string text; };",
        "    std::vector<VisRow> visRows;",
        "    int rowTextX = 0, inputTextX = 0, inputTextY = 0;",
        "    SDL_Rect logArea{0,0,0,0}, inputArea{0,0,0,0};",
        "",
        "    enum class SelRegion { None, History, Input };",
        "    SelRegion selRegion = SelRegion::None;",
        "    bool dragging = false;",
        "",
        "    bool histHasSel = false;",
        "    unsigned long histAnchorSeq = 0, histFocusSeq = 0;",
        "    int histAnchorCol = 0, histFocusCol = 0;",
        "",
        "    bool inHasSel = false;",
        "    int inAnchor = 0, inFocus = 0;",
        "",
        "public:",
        "    void handleMouseDown(int mx, int my, int clicks);",
        "    void handleMouseDrag(int mx, int my);",
        "    void handleMouseUp();",
        "    void copySelection();",
        "    void scroll(float dy);",
        "};",
      ]),

      ...artefactSection("2", "render() — Scroll Resolution (src/ChatBox.cpp)"),
      ...artefactNote("Log geometry + visRows rebuild"),
      ...code([
        "    int logTop = y + PAD;",
        "    int logH = logBottom - logTop;",
        "    int visibleRows = logH > 0 ? logH / ROW_H : 0;",
        "",
        "    rowTextX = x + PAD;",
        "    logArea = { x + 2, logTop, width - 4, logH };",
        "    visRows.clear();",
      ]),
      ...artefactNote("Scroll resolution: anchor seq → index"),
      ...code([
        "        int maxStart = total > 0 ? total - 1 : 0;",
        "        int start;",
        "        if (pinnedToBottom) {",
        "            start = maxTop;",
        "        } else {",
        "            start = maxStart;",
        "            for (int i = 0; i < total; ++i) {",
        "                if (rows[i]->seq >= anchorSeq) { start = i; break; }",
        "            }",
        "        }",
        "",
        "        if (pendingScroll != 0) {",
        "            if (pinnedToBottom) start = maxTop;",
        "            pinnedToBottom = false;",
        "            start += pendingScroll;",
        "            pendingScroll = 0;",
        "        }",
        "",
        "        if (start < 0) start = 0;",
        "        if (start > maxStart) start = maxStart;",
        "        if (!pinnedToBottom) anchorSeq = total > 0 ? rows[start]->seq : 0;",
      ]),

      ...artefactSection("2b", "MessageLog — Cap & Seq ID (src/include/MessageLog.h)"),
      ...code([
        "class MessageLog {",
        "private:",
        "    std::vector<LogEntry> history;",
        "    unsigned long nextSeq = 0;  // assigns monotonic LogEntry::seq",
        "    static constexpr int MAX_ENTRIES = 100;  // in-memory cap",
        "",
        "    void trimHistory() {",
        "        if (history.size() > MAX_ENTRIES) {",
        "            history.erase(history.begin(),",
        "                         history.begin() + (history.size() - MAX_ENTRIES));",
        "        }",
        "    }",
        "};",
        "",
        "struct LogEntry {",
        "    unsigned long seq = 0;  // stable id; survives trim",
        "    std::string timestamp, from, to, text;",
        "    struct State { float posX, posY; std::string domain, action; } state;",
        "};",
      ]),

      ...artefactSection("3", "render() — Selection Highlight (src/ChatBox.cpp)"),
      ...code([
        "        unsigned long loSeq = histAnchorSeq, hiSeq = histFocusSeq;",
        "        int loCol = histAnchorCol, hiCol = histFocusCol;",
        "        if (loSeq > hiSeq || (loSeq == hiSeq && loCol > hiCol)) {",
        "            std::swap(loSeq, hiSeq); std::swap(loCol, hiCol);",
        "        }",
        "",
        "        for (int i = start; i < end; ++i) {",
        "            const LogEntry& e = *rows[i];",
        "            std::string text = formatRow(e);",
        "",
        "            // Selection highlight",
        "            if (histHasSel && e.seq >= loSeq && e.seq <= hiSeq) {",
        "                int len = (int)text.size();",
        "                int a = (e.seq == loSeq) ? loCol : 0;",
        "                int b = (e.seq == hiSeq) ? hiCol : len;",
        "                if (a < 0) a = 0; if (b > len) b = len;",
        "                if (b > a) {",
        "                    const int adv = TextRenderer::ADVANCE * SCALE;",
        "                    SDL_Rect hl{ rowTextX + a * adv, rowY - 1, (b - a) * adv, ROW_H };",
        "                    SDL_SetRenderDrawColor(renderer, 60, 90, 160, 255);",
        "                    SDL_RenderFillRect(renderer, &hl);",
        "                }",
        "            }",
        "",
        "            TextRenderer::draw(renderer, rowTextX, rowY, text, SCALE, 220, 216, 200);",
        "            visRows.push_back({ e.seq, rowY, text });",
        "        }",
      ]),

      ...artefactSection("4", "render() — Input Geometry & Highlight (src/ChatBox.cpp)"),
      ...code([
        "    inputArea = inputRect;",
        "    inputTextX = afterPrompt;",
        "    inputTextY = textY;",
        "",
        "    if (inHasSel) {",
        "        int a = inAnchor, b = inFocus;",
        "        if (a > b) std::swap(a, b);",
        "        const int adv = TextRenderer::ADVANCE * SCALE;",
        "        SDL_Rect hl{ afterPrompt + a * adv, textY - 1, (b - a) * adv, 9 * SCALE };",
        "        SDL_SetRenderDrawColor(renderer, 60, 90, 160, 255);",
        "        SDL_RenderFillRect(renderer, &hl);",
        "    }",
      ]),

      ...artefactSection("5", "Event Handlers (src/ChatBox.cpp)"),
      ...artefactNote("scroll(float dy)"),
      ...code([
        "void ChatBox::scroll(float dy) {",
        "    scrollAccum += dy;",
        "    int step = static_cast<int>(scrollAccum);",
        "    if (step == 0) return;",
        "    scrollAccum -= step;",
        "    pendingScroll += step;  // inverted: > 0 = down (new)",
        "}",
      ]),
      ...artefactNote("handleMouseDown(int mx, int my, int clicks)"),
      ...code([
        "void ChatBox::handleMouseDown(int mx, int my, int clicks) {",
        "    SDL_Point p{ mx, my };",
        "    if (SDL_PointInRect(&p, &inputArea)) {",
        "        selRegion = SelRegion::Input;",
        "        histHasSel = false;",
        "        int col = inputColAt(mx);",
        "        int n = (int)input.size();",
        "        if (clicks >= 3) { inAnchor = 0; inFocus = n; inHasSel = (n > 0); }",
        "        else if (clicks == 2) { int lo, hi; wordBounds(input, col, lo, hi);",
        "            inAnchor = lo; inFocus = hi; inHasSel = (hi > lo); }",
        "        else { cursor = col; inAnchor = inFocus = col; inHasSel = false; dragging = true; }",
        "        return;",
        "    }",
        "    if (SDL_PointInRect(&p, &logArea)) {",
        "        selRegion = SelRegion::History;",
        "        inHasSel = false;",
        "        unsigned long seq; int col;",
        "        histPosAt(mx, my, seq, col);",
        "        const std::string* rowText = nullptr;",
        "        for (const auto& r : visRows) if (r.seq == seq) { rowText = &r.text; break; }",
        "        int n = rowText ? (int)rowText->size() : 0;",
        "        if (clicks >= 3) { histAnchorSeq = histFocusSeq = seq;",
        "            histAnchorCol = 0; histFocusCol = n; histHasSel = (n > 0); }",
        "        else if (clicks == 2 && rowText) { int lo, hi; wordBounds(*rowText, col, lo, hi);",
        "            histAnchorSeq = histFocusSeq = seq;",
        "            histAnchorCol = lo; histFocusCol = hi; histHasSel = (hi > lo); }",
        "        else { histAnchorSeq = histFocusSeq = seq;",
        "            histAnchorCol = histFocusCol = col; histHasSel = false; dragging = true; }",
        "        return;",
        "    }",
        "    histHasSel = inHasSel = false;",
        "}",
      ]),
      ...artefactNote("handleMouseDrag / handleMouseUp"),
      ...code([
        "void ChatBox::handleMouseDrag(int mx, int my) {",
        "    if (!dragging) return;",
        "    if (selRegion == SelRegion::Input) {",
        "        inFocus = inputColAt(mx);",
        "        inHasSel = (inFocus != inAnchor);",
        "    } else if (selRegion == SelRegion::History) {",
        "        unsigned long seq; int col;",
        "        histPosAt(mx, my, seq, col);",
        "        histFocusSeq = seq; histFocusCol = col;",
        "        histHasSel = !(histFocusSeq == histAnchorSeq && histFocusCol == histAnchorCol);",
        "    }",
        "}",
        "",
        "void ChatBox::handleMouseUp() { dragging = false; }",
      ]),

      ...artefactSection("6", "Selection Helpers & Copy (src/ChatBox.cpp)"),
      ...code([
        "int ChatBox::inputColAt(int mx) const {",
        "    const int adv = TextRenderer::ADVANCE * SCALE;",
        "    int col = (mx - inputTextX + adv / 2) / adv;",
        "    if (col < 0) col = 0;",
        "    int n = (int)input.size();",
        "    if (col > n) col = n;",
        "    return col;",
        "}",
        "",
        "void ChatBox::histPosAt(int mx, int my, unsigned long& seq, int& col) const {",
        "    if (visRows.empty()) { seq = 0; col = 0; return; }",
        "    const VisRow* best = &visRows.front();",
        "    for (const auto& r : visRows) {",
        "        if (my >= r.y - 1 && my < r.y - 1 + ROW_H) { best = &r; break; }",
        "        if (my >= r.y) best = &r;",
        "    }",
        "    seq = best->seq;",
        "    const int adv = TextRenderer::ADVANCE * SCALE;",
        "    int c = (mx - rowTextX + adv / 2) / adv;",
        "    int n = (int)best->text.size();",
        "    if (c < 0) c = 0; if (c > n) c = n;",
        "    col = c;",
        "}",
        "",
        "void ChatBox::wordBounds(const std::string& s, int col, int& lo, int& hi) {",
        "    int n = (int)s.size();",
        "    if (n == 0) { lo = hi = 0; return; }",
        "    if (col >= n) col = n - 1;",
        "    auto isWord = [](char c){ return std::isalnum((unsigned char)c) || c == '_'; };",
        "    if (!isWord(s[col])) { lo = col; hi = col + 1; return; }",
        "    lo = col; while (lo > 0 && isWord(s[lo - 1])) lo--;",
        "    hi = col; while (hi < n && isWord(s[hi])) hi++;",
        "}",
        "",
        "void ChatBox::copySelection() {",
        "    std::string out;",
        "    if (selRegion == SelRegion::Input && inHasSel) {",
        "        int a = inAnchor, b = inFocus;",
        "        if (a > b) std::swap(a, b);",
        "        a = std::max(0, a); b = std::min((int)input.size(), b);",
        "        if (b > a) out = input.substr(a, b - a);",
        "    } else if (selRegion == SelRegion::History && histHasSel && world) {",
        "        unsigned long loSeq = histAnchorSeq, hiSeq = histFocusSeq;",
        "        int loCol = histAnchorCol, hiCol = histFocusCol;",
        "        if (loSeq > hiSeq || (loSeq == hiSeq && loCol > hiCol)) {",
        "            std::swap(loSeq, hiSeq); std::swap(loCol, hiCol);",
        "        }",
        "        const std::vector<LogEntry>& hist = world->getMessageLog()->getHistory();",
        "        bool first = true;",
        "        for (const auto& e : hist) {",
        "            if (e.seq < loSeq || e.seq > hiSeq) continue;",
        "            std::string text = formatRow(e);",
        "            int len = (int)text.size();",
        "            int a = (e.seq == loSeq) ? loCol : 0;",
        "            int b = (e.seq == hiSeq) ? hiCol : len;",
        "            if (a < 0) a = 0; if (b > len) b = len;",
        "            if (!first) out += \"\\n\";",
        "            if (b > a) out += text.substr(a, b - a);",
        "            first = false;",
        "        }",
        "    }",
        "    if (!out.empty()) SDL_SetClipboardText(out.c_str());",
        "}",
      ]),

      new Paragraph({ spacing: { before: 180 },
        border: { top: { style: BorderStyle.SINGLE, size: 4, color: "2E75B6", space: 1 } },
        children: [new TextRun({ text: "End of artefact. See also System.cpp for event wiring (MOUSEBUTTONDOWN/MOTION/UP, Cmd+C/Ctrl+C).",
          size: 17, italics: true, color: "555555" })] }),
    ],
  }],
});

Packer.toBuffer(doc).then((buf) => {
  fs.writeFileSync("Textbox_Communication_Log_Manual.docx", buf);
  console.log("written Textbox_Communication_Log_Manual.docx");
});
