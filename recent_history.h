#ifndef RECENT_HISTORY_H
#define RECENT_HISTORY_H

#define RECENT_HISTORY_DEFAULT 10
#define RECENT_HISTORY_MIN 1
#define RECENT_HISTORY_MAX 50

extern int recent_history_count;
void recent_history_load(void);
int recent_history_save(void);

#endif
