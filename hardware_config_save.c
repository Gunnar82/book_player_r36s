#include "hardware_config_save.h"
#include "config_update.h"
#include "storage.h"

#include <stdio.h>

int hardware_config_save_led_gpio(int gpio,int is_manual)
{
    char gpio_value[32];
    snprintf(gpio_value,sizeof(gpio_value),"%d",gpio);

    ConfigUpdate updates[]={
        {"led_gpio",gpio_value},
        {"led_gpio_mode",is_manual?"manual":"auto"}
    };

    return config_update_section(get_storage_config_path(),
                                 "hardware",
                                 updates,
                                 sizeof(updates)/sizeof(updates[0]));
}
