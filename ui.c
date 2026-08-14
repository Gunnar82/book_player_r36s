#include <stdio.h>

#include "ui.h"

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text,
                int x, int y, SDL_Color color)
{
    if (!font)
        return;

    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text, color);
    if (!s)
        return;

    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    if (t) {
        SDL_Rect dst = { x, y, s->w, s->h };
        SDL_RenderCopy(r, t, NULL, &dst);
        SDL_DestroyTexture(t);
    }

    SDL_FreeSurface(s);
}

void draw_text_right(SDL_Renderer *r, TTF_Font *font, const char *text,
                       int right_x, int y, SDL_Color color)
{
    if (!font)
        return;

    int w = 0, h = 0;
    if (TTF_SizeUTF8(font, text, &w, &h) != 0)
        return;

    draw_text(r, font, text, right_x - w, y, color);
}

void format_time(double seconds, char *out, size_t size)
{
    if (seconds < 0)
        seconds = 0;

    int total = (int)seconds;
    int h = total / 3600;
    int m = (total % 3600) / 60;
    int s = total % 60;

    if (h)
        snprintf(out, size, "%d:%02d:%02d", h, m, s);
    else
        snprintf(out, size, "%02d:%02d", m, s);
}
