#ifndef APP_LOG_H
#define APP_LOG_H

#include <stddef.h>

#define APP_LOG_MAX_LINES 120
#define APP_LOG_LINE_LEN  180

void app_log_clear(void);
void app_logf(const char *fmt, ...);
int app_log_count(void);
const char *app_log_line(int index);

#endif
