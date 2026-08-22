#ifndef INPUT_CONFIG_H
#define INPUT_CONFIG_H

#include <SDL2/SDL.h>

void input_config_load(void);
int input_config_is_custom(void);
/* Normalisiert ein SDL-Event auf die interne R36S-Belegung.
 * Rueckgabe 1 = Event weiterverarbeiten, 0 = ignorieren.
 */
int input_normalize_event(SDL_Event *event);


/* D-Pad-Haltewiederholung fuer Menues. Liefert bei Bedarf ein bereits
   normalisiertes SDL_JOYBUTTONDOWN-Event. */
int input_repeat_event(SDL_Event *e, Uint32 now, int enabled);

#endif
