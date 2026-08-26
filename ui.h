#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text,
               int x, int y, SDL_Color color);
void draw_text_right(SDL_Renderer *r, TTF_Font *font, const char *text,
                     int right_x, int y, SDL_Color color);
void format_time(double seconds, char *out, size_t size);

int menu_font_pixels(void);
int menu_line_height(void);
void menu_font_apply(TTF_Font *font);
void menu_font_restore(TTF_Font *font);

/* Gemeinsame Akzentfarbe fuer Auswahltexte und gelbe UI-Balken. */
const char *ui_accent_name(void);
void ui_accent_cycle(int delta);
SDL_Color ui_accent_color(void);
int ui_set_render_draw_color(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

#ifndef UI_IMPLEMENTATION
#define SDL_SetRenderDrawColor(renderer,r,g,b,a) ui_set_render_draw_color((renderer),(r),(g),(b),(a))
#endif

#endif /* UI_H */
