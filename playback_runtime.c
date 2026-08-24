#include "storage.h"
#include "config_update.h"

#include <stdio.h>

/*
 * Die beiden Shutdown-Optionen sind absichtlich Session-Zustand.
 * Alte config.ini-Dateien koennen diese Schluessel noch enthalten; deshalb
 * werden sie nach dem bestehenden Loader explizit zurueckgesetzt.
 */
void __real_load_playback_config(void);

void __wrap_load_playback_config(void)
{
    __real_load_playback_config();
    shutdown_after_tracks=0;
    shutdown_at_book_end=0;
}

/* Nur das Verhalten am Hoerspielende ist persistent. */
int __wrap_save_playback_config(void)
{
    char repeat_value[16];
    snprintf(repeat_value,sizeof(repeat_value),"%d",repeat_book?1:0);

    ConfigUpdate update={"repeat_book",repeat_value};
    return config_update_section(get_storage_config_path(),"playback",&update,1);
}
