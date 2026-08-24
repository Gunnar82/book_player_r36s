#ifndef STREAM_FAVORITES_H
#define STREAM_FAVORITES_H

int stream_favorite_is_set(const char *uuid);
int stream_favorite_toggle(const char *uuid);
void stream_favorites_reload(void);

#endif
