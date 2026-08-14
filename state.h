#ifndef STATE_H
#define STATE_H

#include "types.h"
extern int volume;
extern int idle_timer_minutes;
void setup_state_path(void);
void load_state(void);
void save_state(void);
int find_book_progress(const char *book_path);
int ensure_book_progress(const char *book_path);
void touch_book_progress(int index);
extern BookProgress progress[];
extern int progress_count;

#endif
