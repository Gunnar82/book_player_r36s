#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "config.h"
#include "state.h"
#include "screens/player.h"

/*
 * GPM2804/Batocera has no dedicated volume keys.
 * On the playback screen only, use L1/R1 for volume down/up.
 * All other buttons and all other screens keep their existing mappings.
 */
void __real_player_handle_event(ScreenContext *c, const SDL_Event *e);

void __wrap_player_handle_event(ScreenContext *c, const SDL_Event *e)
{
    if (e && e->type == SDL_JOYBUTTONDOWN) {
        const int b = e->jbutton.button;

        if (b == BUTTON_L1) {
            volume -= VOLUME_STEP;
            if (volume < 0)
                volume = 0;
            Mix_VolumeMusic(volume);
            save_state();
            return;
        }

        if (b == BUTTON_R1) {
            volume += VOLUME_STEP;
            if (volume > MIX_MAX_VOLUME)
                volume = MIX_MAX_VOLUME;
            Mix_VolumeMusic(volume);
            save_state();
            return;
        }
    }

    __real_player_handle_event(c, e);
}
