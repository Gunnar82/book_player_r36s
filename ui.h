#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* Rendert Text an Position (x,y) in gegebener Farbe. Tut nichts,
   falls font == NULL (z.B. Font konnte nicht geladen werden). */
void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text,
                int x, int y, SDL_Color color);

/* Wie draw_text, aber rechtsbündig: "right_x" ist die rechte Kante,
   an der der Text enden soll. */
void draw_text_right(SDL_Renderer *r, TTF_Font *font, const char *text,
                       int right_x, int y, SDL_Color color);

/* Formatiert Sekunden als "m:ss" bzw. "h:mm:ss". */
void format_time(double seconds, char *out, size_t size);

#endif /* UI_H */
