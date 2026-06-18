const fs = require("fs");
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  AlignmentType, LevelFormat, HeadingLevel, BorderStyle, WidthType,
  ShadingType, PageBreak,
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
        children: [new TextRun("ChatBox — Scrolling, Selection, Copy — Instruction Manual")] }),
      new Paragraph({ spacing: { after: 120 }, children: [new TextRun({
        text: "The chat history panel: scrollable message log with inverted mouse wheel, text selection (drag/double/triple-click) in two independent regions (history + input), and clipboard copy via Cmd+C / Ctrl+C.",
        italics: true, size: 18, color: "555555" })] }),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("1. Goal in one sentence")] }),
      new Paragraph({ spacing: { after: 60 }, children: [new TextRun(
        "A scrollable message history with inverted scroll direction, full text selection (characters / words / rows), and clipboard copy support in two independent regions: the history log and the input line.")] }),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("2. Core concepts")] }),
      ...bulletRich([
        { text: "Scroll direction: ", bold: true },
        { text: "wheel/trackpad > 0 moves DOWN (toward newer messages). PgUp/PgDn reversed to match." },
      ]),
      ...bulletRich([
        { text: "Scroll bounds: ", bold: true },
        { text: "User can scroll all the way down until ONLY the last message remains (start index = total - 1). Pinned-to-bottom mode shows the full last page and re-pins when new messages arrive." },
      ]),
      ...bulletRich([
        { text: "Stable anchoring: ", bold: true },
        { text: "History rows are anchored by a per-entry stable sequence ID (seq), not a positional index. Scrolling survives log front-trim (oldest entries culled at MAX_ENTRIES=100) — the anchored message never drifts." },
      ]),
      ...bulletRich([
        { text: "Text selection (history): ", bold: true },
        { text: "Drag to select across characters and rows. Double-click selects a word. Triple-click selects an entire row. Selection is highlighted in blue and independent of input-line selection." },
      ]),
      ...bulletRich([
        { text: "Text selection (input): ", bold: true },
        { text: "Same mechanics: drag / double / triple. Lives on the input line only. Independent of history selection." },
      ]),
      ...bulletRich([
        { text: "Copy to clipboard: ", bold: true },
        { text: "Cmd+C (Mac) or Ctrl+C copies the active selection (whichever region has focus). History selections join rows with newline; input is plain text." },
      ]),
      ...bullet("Scrollbar: visual indicator on the right edge. Thumb color = state: green (live/pinned), amber (manually scrolled)."),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("3. Display — hard rules")] }),
      ...numbered("One row per message: \"HH:MM:SS  from → to: text\". No wrapping."),
      ...numbered("Background color: user messages dark blue, agent messages dark green."),
      ...numbered("Scroll: mouse wheel / trackpad down → scroll down (newer). PgUp = up (older), PgDn = down (newer)."),
      ...numbered("Selected text highlighted blue. Two regions: history (multiline) and input (single line) are independent."),
      ...numbered("Scrollbar on right: green = live (pinned), amber = scrolled (manually positioned). Height = content fraction."),
      ...numbered("Buttons overlay on top-right: no dedicated header row. Buttons float in front of the first message."),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("4. Done when")] }),
      ...bullet("Scroll inverted: mouse wheel > 0 / PgDn goes DOWN (newer). < 0 / PgUp goes UP (older)."),
      ...bullet("Can scroll past the full page down to only the last message visible."),
      ...bullet("Message history stable during scroll: uses seq, not index (survives log trim)."),
      ...bullet("Drag, double-click (word), triple-click (row) all work in both regions."),
      ...bullet("Cmd+C / Ctrl+C copies selection to system clipboard."),
      ...bullet("Scrollbar shows position and state (color)."),

      new Paragraph({ spacing: { before: 140 }, border: { top: { style: BorderStyle.SINGLE, size: 6, color: ACCENT, space: 6 } },
        children: [new TextRun({ text: "Implementation: src/ChatBox.cpp (render + event handling), src/include/ChatBox.h (selection state), src/System.cpp (event wiring).",
          size: 17, italics: true, color: "555555" })] }),

      // ── ARTEFACT ────────────────────────────────────────────────────────────
      new Paragraph({ children: [new PageBreak()] }),

      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Artefact v1 — Verbatim Code", bold: true, size: 32, font: "Arial", color: "1F3864" })] }),
      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Commit: (latest dev)  —  2026-06-18  —  Branch: dev", size: 17, font: "Courier New", color: "555555" })] }),
      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Execution trace: (Env wires events) → System::handleEvents → ChatBox event handlers → render → scroll resolution. Paste sections 1–6 into a clean project to reproduce the approved ChatBox scrolling and selection behaviour exactly.",
        size: 17, italics: true, color: "555555" })] }),
      new Paragraph({ border: { bottom: { style: BorderStyle.SINGLE, size: 4, color: "2E75B6", space: 1 } }, spacing: { before: 0, after: 180 }, children: [] }),

      // Section 1: Header
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
        "    std::vector<VisRow> visRows;    // visible log rows (rebuilt each render)",
        "    int rowTextX = 0;",
        "    int inputTextX = 0, inputTextY = 0;",
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

      // Section 2: Render scroll resolution
      ...artefactSection("2", "render() — Scroll Resolution (src/ChatBox.cpp)"),
      ...artefactNote("Top of render (log geometry + visRows rebuild)"),
      ...code([
        "    int logTop = y + PAD;",
        "    int logBottom = y + height - INPUT_H - PAD;",
        "    int logH = logBottom - logTop;",
        "    int visibleRows = logH > 0 ? logH / ROW_H : 0;",
        "",
        "    rowTextX = x + PAD;",
        "    logArea = { x + 2, logTop, width - 4, logH };",
        "    visRows.clear();",
      ]),
      ...artefactNote("Scroll resolution (anchor point to index)"),
      ...code([
        "        int maxStart = total > 0 ? total - 1 : 0;  // scroll to last msg only",
        "        int start;",
        "        if (pinnedToBottom) {",
        "            start = maxTop;   // live-follow: full last page",
        "        } else {",
        "            start = maxStart;  // scrolled: resolve anchor seq → index",
        "            for (int i = 0; i < total; ++i) {",
        "                if (rows[i]->seq >= anchorSeq) { start = i; break; }",
        "            }",
        "        }",
        "",
        "        // Apply queued wheel steps (inverted: > 0 = down/newer)",
        "        if (pendingScroll != 0) {",
        "            if (pinnedToBottom) start = maxTop;",
        "            pinnedToBottom = false;",
        "            start += pendingScroll;    // inverted: down goes new",
        "            pendingScroll = 0;",
        "        }",
        "",
        "        if (start < 0) start = 0;",
        "        if (start > maxStart) start = maxStart;",
        "        if (!pinnedToBottom) anchorSeq = total > 0 ? rows[start]->seq : 0;",
      ]),

      // Section 3: Render selection highlight + visRows record
      ...artefactSection("3", "render() — Selection Highlight & visRows (src/ChatBox.cpp)"),
      ...artefactNote("Inside row loop: record visRows + draw highlight"),
      ...code([
        "        // Normalized bounds (lo ≤ hi)",
        "        unsigned long loSeq = histAnchorSeq, hiSeq = histFocusSeq;",
        "        int loCol = histAnchorCol, hiCol = histFocusCol;",
        "        if (loSeq > hiSeq || (loSeq == hiSeq && loCol > hiCol)) {",
        "            std::swap(loSeq, hiSeq); std::swap(loCol, hiCol);",
        "        }",
        "        const int adv = TextRenderer::ADVANCE * SCALE;",
        "",
        "        int rowY = logTop;",
        "        for (int i = start; i < end; ++i) {",
        "            const LogEntry& e = *rows[i];",
        "            std::string text = formatRow(e);",
        "",
        "            // Draw row background",
        "            SDL_Rect rowRect{ x + 2, rowY - 1, width - 4, ROW_H };",
        "            if (isUser) SDL_SetRenderDrawColor(renderer, 26, 34, 56, 255);",
        "            else        SDL_SetRenderDrawColor(renderer, 26, 42, 26, 255);",
        "            SDL_RenderFillRect(renderer, &rowRect);",
        "",
        "            // Selection highlight",
        "            if (histHasSel && e.seq >= loSeq && e.seq <= hiSeq) {",
        "                int len = (int)text.size();",
        "                int a = (e.seq == loSeq) ? loCol : 0;",
        "                int b = (e.seq == hiSeq) ? hiCol : len;",
        "                if (a < 0) a = 0; if (b > len) b = len;",
        "                if (b > a) {",
        "                    SDL_Rect hl{ rowTextX + a * adv, rowY - 1, (b - a) * adv, ROW_H };",
        "                    SDL_SetRenderDrawColor(renderer, 60, 90, 160, 255);",
        "                    SDL_RenderFillRect(renderer, &hl);",
        "                }",
        "            }",
        "",
        "            TextRenderer::draw(renderer, rowTextX, rowY, text, SCALE, 220, 216, 200);",
        "            visRows.push_back({ e.seq, rowY, text });",
        "            rowY += ROW_H;",
        "        }",
      ]),

      // Section 4: Input render + highlight
      ...artefactSection("4", "render() — Input Line Geometry & Highlight (src/ChatBox.cpp)"),
      ...code([
        "    std::string prompt = \"> \";",
        "    int textX = inputRect.x + 5;",
        "    int textY = inputRect.y + 6;",
        "    int afterPrompt = TextRenderer::draw(renderer, textX, textY, prompt, SCALE, 150, 170, 230);",
        "",
        "    // Record input geometry",
        "    inputArea = inputRect;",
        "    inputTextX = afterPrompt;",
        "    inputTextY = textY;",
        "",
        "    // Input selection highlight",
        "    if (inHasSel) {",
        "        int a = inAnchor, b = inFocus;",
        "        if (a > b) std::swap(a, b);",
        "        const int adv = TextRenderer::ADVANCE * SCALE;",
        "        SDL_Rect hl{ afterPrompt + a * adv, textY - 1, (b - a) * adv, 9 * SCALE };",
        "        SDL_SetRenderDrawColor(renderer, 60, 90, 160, 255);",
        "        SDL_RenderFillRect(renderer, &hl);",
        "    }",
        "",
        "    TextRenderer::draw(renderer, afterPrompt, textY, input, SCALE, 230, 230, 230);",
      ]),

      // Section 5: scroll() & event handlers
      ...artefactSection("5", "scroll() & Mouse Event Handlers (src/ChatBox.cpp)"),
      ...artefactNote("scroll(float dy) — queue steps for render"),
      ...code([
        "void ChatBox::scroll(float dy) {",
        "    scrollAccum += dy;",
        "    int step = static_cast<int>(scrollAccum);",
        "    if (step == 0) return;",
        "    scrollAccum -= step;",
        "",
        "    // Queue whole-row steps. render() applies them against live layout",
        "    // and re-anchors on the top entry's stable seq.",
        "    pendingScroll += step;  // inverted: step > 0 = down (new)",
        "}",
      ]),
      ...artefactNote("handleMouseDown(int mx, int my, int clicks)"),
      ...code([
        "void ChatBox::handleMouseDown(int mx, int my, int clicks) {",
        "    SDL_Point p{ mx, my };",
        "    if (SDL_PointInRect(&p, &toggleBtn) || SDL_PointInRect(&p, &jumpBtn)) {",
        "        handleClick(mx, my);",
        "        return;",
        "    }",
        "",
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
        "",
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
        "",
        "    histHasSel = inHasSel = false;",
        "    selRegion = SelRegion::None;",
        "}",
      ]),
      ...artefactNote("handleMouseDrag(int mx, int my) — expand selection during drag"),
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
        "void ChatBox::handleMouseUp() {",
        "    dragging = false;",
        "}",
      ]),

      // Section 6: Selection helpers & copy
      ...artefactSection("6", "Selection Helpers & copySelection() (src/ChatBox.cpp)"),
      ...artefactNote("inputColAt(int mx) — map x to character index"),
      ...code([
        "int ChatBox::inputColAt(int mx) const {",
        "    const int adv = TextRenderer::ADVANCE * SCALE;",
        "    int col = (mx - inputTextX + adv / 2) / adv;",
        "    if (col < 0) col = 0;",
        "    int n = (int)input.size();",
        "    if (col > n) col = n;",
        "    return col;",
        "}",
      ]),
      ...artefactNote("histPosAt(int mx, int my, seq&, col&) — map (x,y) to history position"),
      ...code([
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
      ]),
      ...artefactNote("wordBounds(const string& s, int col, lo&, hi&) — find word span"),
      ...code([
        "void ChatBox::wordBounds(const std::string& s, int col, int& lo, int& hi) {",
        "    int n = (int)s.size();",
        "    if (n == 0) { lo = hi = 0; return; }",
        "    if (col >= n) col = n - 1;",
        "    auto isWord = [](char c){ return std::isalnum((unsigned char)c) || c == '_'; };",
        "    if (!isWord(s[col])) { lo = col; hi = col + 1; return; }",
        "    lo = col; while (lo > 0 && isWord(s[lo - 1])) lo--;",
        "    hi = col; while (hi < n && isWord(s[hi])) hi++;",
        "}",
      ]),
      ...artefactNote("copySelection() — to system clipboard"),
      ...code([
        "void ChatBox::copySelection() {",
        "    std::string out;",
        "",
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
        "            bool agentToAgent = (e.from != \"user\" && e.to != \"user\");",
        "            if (!showAgentAgent && agentToAgent) continue;",
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
        "",
        "    if (!out.empty()) SDL_SetClipboardText(out.c_str());",
        "}",
      ]),

      // End note
      new Paragraph({ spacing: { before: 180 },
        border: { top: { style: BorderStyle.SINGLE, size: 4, color: "2E75B6", space: 1 } },
        children: [new TextRun({ text: "End of artefact. Pasting sections 1–6 into a clean project reproduces the approved ChatBox scroll + selection behaviour exactly. See also System.cpp for event wiring (MOUSEBUTTONDOWN/MOTION/UP, Cmd+C/Ctrl+C).",
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

Packer.toBuffer(doc).then((buf) => {
  fs.writeFileSync("ChatBox_Scroll_Selection_Manual.docx", buf);
  console.log("written ChatBox_Scroll_Selection_Manual.docx");
});
