#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <SDL2/SDL.h>
#include <string>

// Minimal dependency-free text drawing using an embedded 8x8 bitmap font
// (public-domain font8x8_basic, ASCII 0x20-0x7E). Each glyph pixel is drawn
// as a scale x scale filled rect. Non-printable / non-ASCII bytes are skipped.
namespace TextRenderer {
    constexpr int GLYPH_W = 8;     // glyph cell width  (px, scale 1)
    constexpr int ADVANCE = 6;     // horizontal advance per char (scale 1)
    constexpr int LINE_H  = 10;    // vertical advance per line (scale 1)

    // Draw a single line of text. Returns the x just past the last glyph.
    int draw(SDL_Renderer* r, int x, int y, const std::string& text,
             int scale, Uint8 cr, Uint8 cg, Uint8 cb);

    // Pixel width of a string at the given scale.
    int width(const std::string& text, int scale);
}

#endif // TEXTRENDERER_H
