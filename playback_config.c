#include "playback_config.h"
#include "config_update.h"
#include "storage.h"

#include <stdio.h>

int playback_config_save_current(void)
{
    char repeat_value[16];
    char tracks_value[32];
    char end_value[16];

    snprintf(repeat_value,sizeof(repeat_value),"%d",repeat_book?1:0);
    snprintf(tracks_value,sizeof(tracks_value),"%d",shutdown_after_tracks);
    snprintf(end_value,sizeof(end_value),"%d",shutdown_at_book_end?1:0);

    ConfigUpdate updates[]={
        {"repeat_book",repeat_value},
        {"shutdown_after_tracks",tracks_value},
        {"shutdown_at_book_end",end_value}
    };

    return config_update_section(get_storage_config_path(),
                                 "playback",
                                 updates,
                                 sizeof(updates)/sizeof(updates[0]));
}
