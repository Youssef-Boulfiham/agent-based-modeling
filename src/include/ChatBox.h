#ifndef CHATBOX_H
#define CHATBOX_H

#include <SDL2/SDL.h>
#include <string>

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
    int scrollOffset = 0;       // rows scrolled up from the bottom (0 = latest)
    int lastTotal = 0;          // filtered row count seen last render (anchor scroll)
    bool showAgentAgent = true; // filter toggle for agent-to-agent chatter
    int userMsgCount = 0;       // rotates which agent a user message targets

    SDL_Rect toggleBtn{0, 0, 0, 0};
    SDL_Rect jumpBtn{0, 0, 0, 0};

    int blinkFrame = 0;         // cursor blink counter

public:
    ChatBox();

    void setWorld(Env* w) { world = w; }
    void render(SDL_Renderer* renderer, int x, int y, int width, int height);

    // Input editing
    void insertText(const char* text);
    void backspace();
    void del();
    void moveCursorLeft();
    void moveCursorRight();
    void submit();

    // Mouse
    void handleClick(int mx, int my);
    void scroll(int dy);
};

#endif
