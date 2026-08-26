#ifndef STATE_H
#define STATE_H

#include "types.h"
extern int volume;
extern int idle_timer_minutes;
extern int display_timeout_seconds;
extern int menu_font_size;
extern int accent_color_index;
extern unsigned long long usage_app_starts;
extern unsigned long long usage_runtime_seconds;
extern unsigned long long usage_playback_seconds;
void setup_state_path(void);
void load_state(void);
void save_state(void);
int find_book_progress(const char *book_path);
int ensure_book_progress(const char *book_path);
void touch_book_progress(int index);
unsigned int ensure_book_dial_id(const char *book_path);
int find_book_progress_by_dial_id(unsigned int dial_id);
extern BookProgress progress[];
extern int progress_count;

#endif
