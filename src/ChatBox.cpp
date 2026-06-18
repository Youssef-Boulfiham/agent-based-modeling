#include "../include/ChatBox.h"
#include "../include/TextRenderer.h"
#include "../include/Env.h"
#include "../include/MessageLog.h"
#include <vector>

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

    SDL_SetRenderDrawColor(renderer, 45, 45, 60, 255);
    SDL_RenderFillRect(renderer, &toggleBtn);
    SDL_RenderFillRect(renderer, &jumpBtn);
    SDL_SetRenderDrawColor(renderer, 120, 120, 170, 255);
    SDL_RenderDrawRect(renderer, &toggleBtn);
    SDL_RenderDrawRect(renderer, &jumpBtn);
    TextRenderer::draw(renderer, toggleBtn.x + 5, toggleBtn.y + 4, toggleLabel, SCALE, 220, 220, 230);
    TextRenderer::draw(renderer, jumpBtn.x + 5, jumpBtn.y + 4, jumpLabel, SCALE, 220, 220, 230);

    // ── Log area geometry ───────────────────────────────────────────────────
    int logTop    = y + PAD + BTN_H + 4;
    int logBottom = y + height - INPUT_H - PAD;
    int logH      = logBottom - logTop;
    int visibleRows = logH > 0 ? logH / ROW_H : 0;

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

        // Anchor the view: if new messages arrived while the user is scrolled up
        // (scrollOffset > 0), push the offset up by the same amount so the rows
        // being read stay put instead of sliding down.
        if (scrollOffset > 0 && total > lastTotal) {
            scrollOffset += total - lastTotal;
        }
        lastTotal = total;

        if (scrollOffset < 0) scrollOffset = 0;
        int maxOffset = total > visibleRows ? total - visibleRows : 0;
        if (scrollOffset > maxOffset) scrollOffset = maxOffset;

        int start = total - visibleRows - scrollOffset;
        if (start < 0) start = 0;
        int end = start + visibleRows;
        if (end > total) end = total;

        int rowY = logTop;
        for (int i = start; i < end; ++i) {
            const LogEntry& e = *rows[i];
            bool isUser = (e.from == "user");

            // Row background distinguishes sender (user blue, agent green).
            SDL_Rect rowRect{ x + 2, rowY - 1, width - 4, ROW_H };
            if (isUser) SDL_SetRenderDrawColor(renderer, 26, 34, 56, 255);
            else        SDL_SetRenderDrawColor(renderer, 26, 42, 26, 255);
            SDL_RenderFillRect(renderer, &rowRect);

            TextRenderer::draw(renderer, x + PAD, rowY, formatRow(e), SCALE, 220, 216, 200);
            rowY += ROW_H;
        }
    }

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
    scrollOffset = 0; // snap to latest after sending
}

void ChatBox::handleClick(int mx, int my) {
    SDL_Point p{ mx, my };
    if (SDL_PointInRect(&p, &toggleBtn)) {
        showAgentAgent = !showAgentAgent;
        return;
    }
    if (SDL_PointInRect(&p, &jumpBtn)) {
        scrollOffset = 0;
        return;
    }
}

void ChatBox::scroll(int dy) {
    // dy > 0 = wheel up = look back in history.
    scrollOffset += dy;
    if (scrollOffset < 0) scrollOffset = 0;
}
