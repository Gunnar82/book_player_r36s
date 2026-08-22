#ifndef INPUT_CONFIG_H
#define INPUT_CONFIG_H

#include <SDL2/SDL.h>

void input_config_load(void);
int input_config_is_custom(void);
/* Normalisiert ein SDL-Event auf die interne R36S-Belegung.
 * Rueckgabe 1 = Event weiterverarbeiten, 0 = ignorieren.
 */
int input_normalize_event(SDL_Event *event);

/* Transparenter Poll-Wrapper: damit bleibt main.c kompatibel, und die
 * Eingaben werden vor der bisherigen R36S-Logik normalisiert. */
int input_poll_event(SDL_Event *event);
#ifndef INPUT_CONFIG_NO_POLL_WRAP
#define SDL_PollEvent input_poll_event
#endif

#endif
