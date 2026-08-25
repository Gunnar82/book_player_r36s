#include "input_config.h"
#include "config.h"
#include "screens.h"
#include <SDL2/SDL.h>

/*
 * The main event loop uses BUTTON_X globally to open the system menu before
 * the Bluetooth screen sees the event.  While the Bluetooth UI is active we
 * remap that button to the same internal removal action used by the Batocera
 * Bluetooth UI, so the face button remains local to this screen.
 */
static int bluetooth_input_active;

void r36s_bluetooth_input_set_active(int active)
{
    bluetooth_input_active=active?1:0;
}

int __real_input_normalize_event(SDL_Event *e);

int __wrap_input_normalize_event(SDL_Event *e)
{
    int rc=__real_input_normalize_event(e);
    if(!rc||!bluetooth_input_active||!e)return rc;
    if((e->type==SDL_JOYBUTTONDOWN||e->type==SDL_JOYBUTTONUP) &&
       e->jbutton.button==BUTTON_X)
        e->jbutton.button=BUTTON_R1;
    return rc;
}
