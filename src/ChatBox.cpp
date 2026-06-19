#include "../include/ChatBox.h"
#include "../include/TextRenderer.h"
#include "../include/Env.h"
#include "../include/MessageLog.h"
#include "../include/TextRenderer.h"
#include <vector>
#include <algorithm>
#include <cctype>

namespace {
    constexpr int PAD       = 6;    // inner padding
    constexpr int ROW_H     = 11;   // height of one log row (px)
    constexpr int INPUT_H   = 20;   // height of the input line area
    constexpr int BTN_H     = 16;   // header button height
    constexpr int SCALE     = 1;    // font scale for log + input
}

ChatBox::ChatBox() {}

// Build the single-line display string for an entry: "HH:MM:SS from -> to: text"
static std::string formatRow(const LogEntry& e) {
    return e.timestamp + " " + e.from + " -> " + e.to + ": " + e.text;
}

void ChatBox::render(SDL_Renderer* renderer, int x, int y, int width, int height) {
    panelArea = {x, y, width, height};
    blinkFrame++;

    // Panel background + border
    SDL_SetRenderDrawColor(renderer, 14, 14, 22, 255);
    SDL_RenderFillRect(renderer, &panelArea);
    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
    SDL_RenderDrawRect(renderer, &panelArea);

    // ── Header buttons (top-right): toggle filter + jump to bottom ──────────
    int btnY = y + 3;
    std::string toggleLabel = showAgentAgent ? "HIDE A-A" : "SHOW A-A";
    int toggleW = TextRenderer::width(toggleLabel, SCALE) + 10;
    std::string jumpLabel = "BOTTOM";
    int jumpW = TextRenderer::width(jumpLabel, SCALE) + 10;

    jumpBtn   = { x + width - PAD - jumpW, btnY, jumpW, BTN_H };
    toggleBtn = { jumpBtn.x - 8 - toggleW, btnY, toggleW, BTN_H };
    // Buttons drawn LATER as an overlay (after the log) so they float in front of
    // the top message row — saves the vertical strip a dedicated header would cost.

    // ── Log area geometry ───────────────────────────────────────────────────
    int logTop    = y + PAD;
    int logBottom = y + height - INPUT_H - PAD;
    int logH      = logBottom - logTop;
    int visibleRows = logH > 0 ? logH / ROW_H : 0;

    rowTextX = x + PAD;
    logArea  = { x + 2, logTop, width - 4, logH };
    visRows.clear();

    if (world && world->getMessageLog()) {
        const std::vector<LogEntry>& hist = world->getMessageLog()->getHistory();

        // Filtered view: optionally drop agent-to-agent messages.
        std::vector<const LogEntry*> rows;
        rows.reserve(hist.size());
        for (const auto& e : hist) {
            bool agentToAgent = (e.from != "user" && e.to != "user");
            if (!showAgentAgent && agentToAgent) continue;
            rows.push_back(&e);
        }

        int total = static_cast<int>(rows.size());
        int maxTop = total > visibleRows ? total - visibleRows : 0;
        lastMaxTop = maxTop;   // remember the latest page top for scroll()

        // Resolve the first visible row from the scroll model.
        //   pinned  -> base case: show the most recent page (follows new msgs).
        //   scrolled-> hold anchorSeq: the STABLE id of the top entry. Resolved to
        //              an index each frame, so neither appends at the bottom nor
        //              front-trims of the log move the rows being read.
        // Scroll can go all the way down until ONLY the last message remains
        // (start = total-1), past the full last page.
        int maxStart = total > 0 ? total - 1 : 0;

        int start;
        if (pinnedToBottom) {
            start = maxTop;   // live-follow: show the full last page
        } else {
            // First row whose seq >= anchorSeq (the anchored entry, or its
            // nearest survivor if it was trimmed off the front).
            start = maxStart;
            for (int i = 0; i < total; ++i) {
                if (rows[i]->seq >= anchorSeq) { start = i; break; }
            }
        }

        // Apply queued wheel steps now that we know the row layout.
        // (inverted scroll direction: step > 0 moves toward newer/bottom)
        if (pendingScroll != 0) {
            if (pinnedToBottom) start = maxTop;  // detach from live-follow base
            pinnedToBottom = false;              // any manual scroll leaves follow
            start += pendingScroll;
            pendingScroll = 0;
        }

        if (start < 0) start = 0;
        if (start > maxStart) start = maxStart;
        if (!pinnedToBottom) anchorSeq = total > 0 ? rows[start]->seq : 0;

        int end = start + visibleRows;
        if (end > total) end = total;

        // Normalized history selection bounds (lo <= hi by seq, then col).
        unsigned long loSeq = histAnchorSeq, hiSeq = histFocusSeq;
        int loCol = histAnchorCol, hiCol = histFocusCol;
        if (loSeq > hiSeq || (loSeq == hiSeq && loCol > hiCol)) {
            std::swap(loSeq, hiSeq); std::swap(loCol, hiCol);
        }
        const int adv = TextRenderer::ADVANCE * SCALE;

        int rowY = logTop;
        for (int i = start; i < end; ++i) {
            const LogEntry& e = *rows[i];
            bool isUser = (e.from == "user");
            std::string text = formatRow(e);

            // Row background distinguishes sender (user blue, agent green).
            SDL_Rect rowRect{ x + 2, rowY - 1, width - 4, ROW_H };
            if (isUser) SDL_SetRenderDrawColor(renderer, 26, 34, 56, 255);
            else        SDL_SetRenderDrawColor(renderer, 26, 42, 26, 255);
            SDL_RenderFillRect(renderer, &rowRect);

            // Selection highlight for this row, if its seq is in range.
            if (histHasSel && e.seq >= loSeq && e.seq <= hiSeq) {
                int len = static_cast<int>(text.size());
                int a = (e.seq == loSeq) ? loCol : 0;
                int b = (e.seq == hiSeq) ? hiCol : len;
                if (a < 0) a = 0; if (b > len) b = len;
                if (b > a) {
                    SDL_Rect hl{ rowTextX + a * adv, rowY - 1, (b - a) * adv, ROW_H };
                    SDL_SetRenderDrawColor(renderer, 60, 90, 160, 255);
                    SDL_RenderFillRect(renderer, &hl);
                }
            }

            TextRenderer::draw(renderer, rowTextX, rowY, text, SCALE, 220, 216, 200);
            visRows.push_back({ e.seq, rowY, text });
            rowY += ROW_H;
        }

        // ── Scrollbar (right edge of log area) ──────────────────────────────
        // Shown only when content overflows. Thumb size = visible fraction,
        // thumb position = start / maxTop. Highlighted when actively scrolled.
        if (total > visibleRows && logH > 0) {
            const int SB_W = 3 * SCALE;
            int trackX = x + width - SB_W - 2;
            SDL_Rect track{ trackX, logTop, SB_W, logH };
            SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
            SDL_RenderFillRect(renderer, &track);

            float frac    = static_cast<float>(visibleRows) / total;
            int thumbH    = static_cast<int>(logH * frac);
            if (thumbH < 8) thumbH = 8;            // keep grabbable/visible
            int travel    = logH - thumbH;
            float pos     = maxTop > 0 ? static_cast<float>(start) / maxTop : 0.0f;
            int thumbY    = logTop + static_cast<int>(travel * pos);

            SDL_Rect thumb{ trackX, thumbY, SB_W, thumbH };
            if (pinnedToBottom) SDL_SetRenderDrawColor(renderer, 120, 210, 130, 255); // LIVE green
            else                SDL_SetRenderDrawColor(renderer, 230, 200, 110, 255); // SCROLLED amber
            SDL_RenderFillRect(renderer, &thumb);
        }
    }

    // ── Header buttons overlay (float in front of the top message row) ───────
    SDL_SetRenderDrawColor(renderer, 45, 45, 60, 255);
    SDL_RenderFillRect(renderer, &toggleBtn);
    SDL_RenderFillRect(renderer, &jumpBtn);
    SDL_SetRenderDrawColor(renderer, 120, 120, 170, 255);
    SDL_RenderDrawRect(renderer, &toggleBtn);
    SDL_RenderDrawRect(renderer, &jumpBtn);
    TextRenderer::draw(renderer, toggleBtn.x + 5, toggleBtn.y + 4, toggleLabel, SCALE, 220, 220, 230);
    TextRenderer::draw(renderer, jumpBtn.x + 5, jumpBtn.y + 4, jumpLabel, SCALE, 220, 220, 230);

    // ── Input line (bottom) ─────────────────────────────────────────────────
    SDL_Rect inputRect{ x + 2, y + height - INPUT_H, width - 4, INPUT_H - 2 };
    SDL_SetRenderDrawColor(renderer, 35, 35, 44, 255);
    SDL_RenderFillRect(renderer, &inputRect);
    SDL_SetRenderDrawColor(renderer, 90, 110, 200, 255);
    SDL_RenderDrawRect(renderer, &inputRect);

    std::string prompt = "> ";
    int textX = inputRect.x + 5;
    int textY = inputRect.y + 6;
    int afterPrompt = TextRenderer::draw(renderer, textX, textY, prompt, SCALE, 150, 170, 230);

    // Record input geometry for hit-testing/selection.
    inputArea  = inputRect;
    inputTextX = afterPrompt;
    inputTextY = textY;

    // Input selection highlight (drawn under the text).
    if (inHasSel) {
        int a = inAnchor, b = inFocus;
        if (a > b) std::swap(a, b);
        const int adv = TextRenderer::ADVANCE * SCALE;
        SDL_Rect hl{ afterPrompt + a * adv, textY - 1, (b - a) * adv, 9 * SCALE };
        SDL_SetRenderDrawColor(renderer, 60, 90, 160, 255);
        SDL_RenderFillRect(renderer, &hl);
    }

    TextRenderer::draw(renderer, afterPrompt, textY, input, SCALE, 230, 230, 230);

    // Blinking caret at the cursor position.
    if ((blinkFrame / 30) % 2 == 0) {
        std::string before = input.substr(0, cursor);
        int caretX = afterPrompt + TextRenderer::width(before, SCALE);
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_Rect caret{ caretX, textY - 1, 1 * SCALE, 9 * SCALE };
        SDL_RenderFillRect(renderer, &caret);
    }
}

void ChatBox::insertText(const char* text) {
    if (!text) return;
    std::string ins;
    // Keep only printable ASCII (the bitmap font covers 0x20..0x7E).
    for (const char* p = text; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 0x20 && c <= 0x7E) ins.push_back(static_cast<char>(c));
    }
    if (ins.empty()) return;
    input.insert(cursor, ins);
    cursor += static_cast<int>(ins.size());
}

void ChatBox::backspace() {
    if (cursor > 0) {
        input.erase(cursor - 1, 1);
        cursor--;
    }
}

void ChatBox::del() {
    if (cursor < static_cast<int>(input.size())) {
        input.erase(cursor, 1);
    }
}

void ChatBox::moveCursorLeft() {
    if (cursor > 0) cursor--;
}

void ChatBox::moveCursorRight() {
    if (cursor < static_cast<int>(input.size())) cursor++;
}

void ChatBox::submit() {
    if (input.empty() || !world) return;
    int active = world->getActiveAgents();
    int agentId = active > 0 ? (userMsgCount % active) : 0;
    world->queueUserInput(input, agentId);
    userMsgCount++;
    input.clear();
    cursor = 0;
    pinnedToBottom = true; // snap to latest after sending
}

void ChatBox::handleClick(int mx, int my) {
    SDL_Point p{ mx, my };
    if (SDL_PointInRect(&p, &toggleBtn)) {
        showAgentAgent = !showAgentAgent;
        return;
    }
    if (SDL_PointInRect(&p, &jumpBtn)) {
        pinnedToBottom = true; // jump to bottom: follow the latest again
        return;
    }
}

// ── Selection: position mapping ─────────────────────────────────────────────
int ChatBox::inputColAt(int mx) const {
    const int adv = TextRenderer::ADVANCE * SCALE;
    int col = (mx - inputTextX + adv / 2) / adv;   // round to nearest gap
    if (col < 0) col = 0;
    int n = static_cast<int>(input.size());
    if (col > n) col = n;
    return col;
}

void ChatBox::histPosAt(int mx, int my, unsigned long& seq, int& col) const {
    if (visRows.empty()) { seq = 0; col = 0; return; }
    // Nearest visible row by y (clamp above-first / below-last).
    const VisRow* best = &visRows.front();
    for (const auto& r : visRows) {
        if (my >= r.y - 1 && my < r.y - 1 + ROW_H) { best = &r; break; }
        if (my >= r.y) best = &r;   // track last row whose top is above the cursor
    }
    seq = best->seq;
    const int adv = TextRenderer::ADVANCE * SCALE;
    int c = (mx - rowTextX + adv / 2) / adv;
    int n = static_cast<int>(best->text.size());
    if (c < 0) c = 0; if (c > n) c = n;
    col = c;
}

void ChatBox::wordBounds(const std::string& s, int col, int& lo, int& hi) {
    int n = static_cast<int>(s.size());
    if (n == 0) { lo = hi = 0; return; }
    if (col >= n) col = n - 1;
    auto isWord = [](char c){ return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
    if (!isWord(s[col])) { lo = col; hi = col + 1; return; }
    lo = col; while (lo > 0 && isWord(s[lo - 1])) lo--;
    hi = col; while (hi < n && isWord(s[hi])) hi++;
}

// ── Selection: mouse handling ───────────────────────────────────────────────
void ChatBox::handleMouseDown(int mx, int my, int clicks) {
    SDL_Point p{ mx, my };

    // Buttons take priority and never start a selection.
    if (SDL_PointInRect(&p, &toggleBtn) || SDL_PointInRect(&p, &jumpBtn)) {
        handleClick(mx, my);
        return;
    }

    if (SDL_PointInRect(&p, &inputArea)) {
        // ── Input region (independent of history) ──
        inputFocused = true;               // arrows now edit text, not pan camera
        selRegion = SelRegion::Input;
        histHasSel = false;
        int col = inputColAt(mx);
        int n = static_cast<int>(input.size());
        if (clicks >= 3) {                       // triple-click: whole line
            inAnchor = 0; inFocus = n; inHasSel = (n > 0); dragging = false;
        } else if (clicks == 2) {                // double-click: word
            int lo, hi; wordBounds(input, col, lo, hi);
            inAnchor = lo; inFocus = hi; inHasSel = (hi > lo); dragging = false;
        } else {                                 // single: place caret, start drag
            cursor = col; inAnchor = inFocus = col; inHasSel = false; dragging = true;
        }
        return;
    }

    if (SDL_PointInRect(&p, &logArea)) {
        // ── History region (independent of input) ──
        inputFocused = false;              // clicking the log releases the input
        selRegion = SelRegion::History;
        inHasSel = false;
        unsigned long seq; int col;
        histPosAt(mx, my, seq, col);
        // Find the row text for word/line ops.
        const std::string* rowText = nullptr;
        for (const auto& r : visRows) if (r.seq == seq) { rowText = &r.text; break; }
        int n = rowText ? static_cast<int>(rowText->size()) : 0;
        if (clicks >= 3) {                       // triple: whole row
            histAnchorSeq = histFocusSeq = seq;
            histAnchorCol = 0; histFocusCol = n; histHasSel = (n > 0); dragging = false;
        } else if (clicks == 2 && rowText) {     // double: word
            int lo, hi; wordBounds(*rowText, col, lo, hi);
            histAnchorSeq = histFocusSeq = seq;
            histAnchorCol = lo; histFocusCol = hi; histHasSel = (hi > lo); dragging = false;
        } else {                                 // single: start drag
            histAnchorSeq = histFocusSeq = seq;
            histAnchorCol = histFocusCol = col; histHasSel = false; dragging = true;
        }
        return;
    }

    // Clicked elsewhere: clear selections and release input focus.
    inputFocused = false;
    histHasSel = inHasSel = false;
    selRegion = SelRegion::None;
}

void ChatBox::handleMouseDrag(int mx, int my) {
    if (!dragging) return;
    if (selRegion == SelRegion::Input) {
        inFocus = inputColAt(mx);
        inHasSel = (inFocus != inAnchor);
    } else if (selRegion == SelRegion::History) {
        unsigned long seq; int col;
        histPosAt(mx, my, seq, col);
        histFocusSeq = seq; histFocusCol = col;
        histHasSel = !(histFocusSeq == histAnchorSeq && histFocusCol == histAnchorCol);
    }
}

void ChatBox::handleMouseUp() {
    dragging = false;
}

// ── Selection: copy to clipboard ────────────────────────────────────────────
void ChatBox::copySelection() {
    std::string out;

    if (selRegion == SelRegion::Input && inHasSel) {
        int a = inAnchor, b = inFocus;
        if (a > b) std::swap(a, b);
        a = std::max(0, a); b = std::min<int>(b, input.size());
        if (b > a) out = input.substr(a, b - a);
    } else if (selRegion == SelRegion::History && histHasSel && world && world->getMessageLog()) {
        unsigned long loSeq = histAnchorSeq, hiSeq = histFocusSeq;
        int loCol = histAnchorCol, hiCol = histFocusCol;
        if (loSeq > hiSeq || (loSeq == hiSeq && loCol > hiCol)) {
            std::swap(loSeq, hiSeq); std::swap(loCol, hiCol);
        }
        // Rebuild the same filtered row set used for display, pull seq range.
        const std::vector<LogEntry>& hist = world->getMessageLog()->getHistory();
        bool first = true;
        for (const auto& e : hist) {
            bool agentToAgent = (e.from != "user" && e.to != "user");
            if (!showAgentAgent && agentToAgent) continue;
            if (e.seq < loSeq || e.seq > hiSeq) continue;
            std::string text = formatRow(e);
            int len = static_cast<int>(text.size());
            int a = (e.seq == loSeq) ? loCol : 0;
            int b = (e.seq == hiSeq) ? hiCol : len;
            if (a < 0) a = 0; if (b > len) b = len;
            if (!first) out += "\n";
            if (b > a) out += text.substr(a, b - a);
            first = false;
        }
    }

    if (!out.empty()) SDL_SetClipboardText(out.c_str());
}

void ChatBox::scroll(float dy) {
    // Accumulate fractional wheel deltas (a trackpad sends many tiny values, and
    // wheel.y is often 0). Act only on whole-row steps; the accumulator cancels
    // jitter.
    scrollAccum += dy;
    int step = static_cast<int>(scrollAccum);   // truncate toward zero
    if (step == 0) return;
    scrollAccum -= step;

    // Queue whole-row steps. render() applies them against the current row layout
    // and re-anchors on the top entry's stable seq (or re-pins if it lands on the
    // latest page). Doing it there keeps the anchor immune to log front-trims.
    pendingScroll += step;            // step > 0 = up = older
}

// ── Event routing ───────────────────────────────────────────────────────────
// One entry point for all chat-panel input. System forwards every non-system
// SDL event here; types the panel does not care about fall through harmlessly.
void ChatBox::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_TEXTINPUT:
            // Printable characters typed into the chat input.
            insertText(event.text.text);
            break;

        case SDL_KEYDOWN:
            // Copy selection: Cmd+C (mac) / Ctrl+C.
            if (event.key.keysym.sym == SDLK_c &&
                (event.key.keysym.mod & (KMOD_GUI | KMOD_CTRL))) {
                copySelection();
                break;
            }
            switch (event.key.keysym.sym) {
                case SDLK_BACKSPACE: backspace();       break;
                case SDLK_DELETE:    del();             break;
                case SDLK_LEFT:      moveCursorLeft();  break;
                case SDLK_RIGHT:     moveCursorRight(); break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:  submit();          break;
                // Keyboard scroll (reliable, independent of mouse wheel).
                case SDLK_PAGEUP:    scroll(3.0f);      break;
                case SDLK_PAGEDOWN:  scroll(-3.0f);     break;
                default: break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
                handleMouseDown(event.button.x, event.button.y, event.button.clicks);
            break;

        case SDL_MOUSEMOTION:
            if (event.motion.state & SDL_BUTTON_LMASK)
                handleMouseDrag(event.motion.x, event.motion.y);
            break;

        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT)
                handleMouseUp();
            break;

        case SDL_MOUSEWHEEL: {
            // Precise delta (trackpads report fractions; wheel.y often 0).
            // Normalize natural-scroll so dy > 0 is always "up".
            float dy = event.wheel.preciseY != 0.0f
                         ? event.wheel.preciseY
                         : static_cast<float>(event.wheel.y);
            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) dy = -dy;
            scroll(dy);
            break;
        }

        default:
            break;
    }
}
