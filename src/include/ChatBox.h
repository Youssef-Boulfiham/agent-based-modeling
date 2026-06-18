#ifndef CHATBOX_H
#define CHATBOX_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class Env;

// Interactive chat panel: renders the message history (from Env's MessageLog)
// and an editable input line with a working cursor. User input is routed to
// Env::queueUserInput so it enters the priority queue (highest priority).
class ChatBox {
private:
    SDL_Rect panelArea{0, 0, 0, 0};
    Env* world = nullptr;

    std::string input;          // current input buffer
    int cursor = 0;             // cursor index within input [0..input.size()]

    // Scroll model: anchor on a STABLE per-entry seq, not a positional index.
    // When pinned, the view follows the latest message (base case). When the user
    // scrolls up, pinned=false and anchorSeq holds the seq of the entry at the top
    // of the viewport. Seq is monotonic and survives the log's front-trim, so new
    // messages (appended OR trimmed off the front) never move the rows being read.
    bool pinnedToBottom = true;     // true = follow latest; false = locked at anchorSeq
    unsigned long anchorSeq = 0;    // seq of the entry shown at the top of the viewport
    int  lastMaxTop = 0;            // top index of the latest page (updated each render)
    float scrollAccum = 0.0f;   // fractional wheel delta accumulator (trackpad)
    int  pendingScroll = 0;     // whole-row steps queued by scroll(), applied in render

    bool showAgentAgent = true; // filter toggle for agent-to-agent chatter
    int userMsgCount = 0;       // rotates which agent a user message targets

    SDL_Rect toggleBtn{0, 0, 0, 0};
    SDL_Rect jumpBtn{0, 0, 0, 0};

    int blinkFrame = 0;         // cursor blink counter

    // ── Text selection ──────────────────────────────────────────────────────
    // Two INDEPENDENT selection regions: the message-history log and the input
    // line. Selecting in one clears the other. History positions are stored by
    // (seq, col) so they stay stable across scrolling/trimming; input positions
    // are plain column indices into `input`.
    struct VisRow { unsigned long seq; int y; std::string text; };
    std::vector<VisRow> visRows;   // rebuilt every render (visible log rows)
    int rowTextX = 0;              // x where row text starts
    int inputTextX = 0;            // x where input text starts (after the "> ")
    int inputTextY = 0;
    SDL_Rect logArea{0, 0, 0, 0};
    SDL_Rect inputArea{0, 0, 0, 0};

    enum class SelRegion { None, History, Input };
    SelRegion selRegion = SelRegion::None;
    bool dragging = false;

    bool histHasSel = false;
    unsigned long histAnchorSeq = 0, histFocusSeq = 0;
    int histAnchorCol = 0, histFocusCol = 0;

    bool inHasSel = false;
    int inAnchor = 0, inFocus = 0;

    // Map a mouse position to a logical text position within each region.
    void histPosAt(int mx, int my, unsigned long& seq, int& col) const;
    int  inputColAt(int mx) const;
    static void wordBounds(const std::string& s, int col, int& lo, int& hi);

public:
    ChatBox();

    void setWorld(Env* w) { world = w; }
    void render(SDL_Renderer* renderer, int x, int y, int width, int height);

    // Single entry point: route one SDL event (text input, editing keys, mouse,
    // wheel) to the right handler. System forwards every non-system event here.
    void handleEvent(const SDL_Event& event);

    // Input editing
    void insertText(const char* text);
    void backspace();
    void del();
    void moveCursorLeft();
    void moveCursorRight();
    void submit();

    // Mouse
    void handleClick(int mx, int my);
    void handleMouseDown(int mx, int my, int clicks);
    void handleMouseDrag(int mx, int my);
    void handleMouseUp();
    void copySelection();    // -> system clipboard (Cmd/Ctrl+C)
    void scroll(float dy);
};

#endif
