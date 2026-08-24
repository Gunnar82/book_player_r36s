#include "download_config_save.h"
#include "config_update.h"
#include "storage.h"

#include <stdio.h>

int download_config_save_enabled(int enabled)
{
    char value[8];
    snprintf(value,sizeof(value),"%d",enabled?1:0);
    ConfigUpdate update={"enabled",value};
    return config_update_section(get_storage_config_path(),"download",&update,1);
}
