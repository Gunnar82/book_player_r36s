#ifndef SCREEN_SLEEP_TIMER_COMPAT_H
#define SCREEN_SLEEP_TIMER_COMPAT_H
#include "systemmenu.h"
#define sleeptimer_handle_event systemmenu_handle_event
#define sleeptimer_render systemmenu_render
#endif
