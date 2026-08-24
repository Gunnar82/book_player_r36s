#include "stream_config.h"
#include "streaming.h"
#include "storage.h"
#include "config_update.h"

int stream_config_save_cert_mode(void)
{
    ConfigUpdate updates[]={
        {"client_cert_mode",streaming_cert_mode_name()}
    };
    return config_update_section(get_storage_config_path(),"streams",updates,sizeof(updates)/sizeof(updates[0]));
}
