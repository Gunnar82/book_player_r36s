#include "battery_bluez.h"
#include "app_log.h"

#include <systemd/sd-bus.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bluetooth.h"

#define BLUEZ_SERVICE "org.bluez"
#define BLUEZ_ADAPTER_PATH "/org/bluez/hci0"
#define PROVIDER_PATH "/de/gunnar/HoerspielPlayer/BatteryProvider"
#define BATTERY_PATH PROVIDER_PATH "/battery0"
#define REGISTER_RETRY_SECONDS 5

struct BatteryBluez {
    sd_bus *bus;
    sd_bus_slot *slot;
    unsigned char percent;
    int percent_valid;
    int registered;
    time_t last_register_try;
};

static int get_percentage(sd_bus *bus,
                          const char *path,
                          const char *interface,
                          const char *property,
                          sd_bus_message *reply,
                          void *userdata,
                          sd_bus_error *ret_error)
{
    if(!bluetooth_service_available()) return -1;
    (void)bus;
    (void)path;
    (void)interface;
    (void)property;
    (void)ret_error;

    BatteryBluez *provider = (BatteryBluez *)userdata;
    unsigned char value = provider && provider->percent_valid
                            ? provider->percent
                            : 0;

    return sd_bus_message_append(reply, "y", value);
}

static const sd_bus_vtable battery_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_PROPERTY("Percentage",
                    "y",
                    get_percentage,
                    0,
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_VTABLE_END
};

static int register_provider(BatteryBluez *provider)
{
    if (!provider || !provider->bus)
        return -1;

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;

    int r = sd_bus_call_method(provider->bus,
                               BLUEZ_SERVICE,
                               BLUEZ_ADAPTER_PATH,
                               "org.bluez.BatteryProviderManager1",
                               "RegisterBatteryProvider",
                               &error,
                               &reply,
                               "o",
                               PROVIDER_PATH);

    if (r >= 0) {
        if (!provider->registered)
            app_logf("BlueZ Akku: Provider registriert");
        provider->registered = 1;
    } else if (sd_bus_error_has_name(&error, "org.bluez.Error.AlreadyExists")) {
        provider->registered = 1;
    } else {
        if (provider->registered)
            app_logf("BlueZ Akku: Registrierung verloren");
        provider->registered = 0;
        if (error.name && error.name[0])
            app_logf("BlueZ Akku Fehler: %s", error.name);
        if (error.message && error.message[0])
            app_logf("BlueZ Akku Text: %s", error.message);
    }

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);
    return r;
}

int battery_bluez_init(BatteryBluez **out, int percent)
{
    if (!out)
        return -1;

    *out = NULL;

    BatteryBluez *provider = calloc(1, sizeof(*provider));
    if (!provider)
        return -1;

    if (percent >= 0) {
        if (percent > 100)
            percent = 100;
        provider->percent = (unsigned char)percent;
        provider->percent_valid = 1;
    }

    int r = sd_bus_open_system(&provider->bus);
    if (r < 0) {
        app_logf("BlueZ Akku: System-D-Bus nicht erreichbar (%d)", r);
        free(provider);
        return r;
    }

    r = sd_bus_add_object_vtable(provider->bus,
                                 &provider->slot,
                                 BATTERY_PATH,
                                 "org.bluez.BatteryProvider1",
                                 battery_vtable,
                                 provider);
    if (r < 0) {
        app_logf("BlueZ Akku: D-Bus Objekt fehlgeschlagen (%d)", r);
        sd_bus_unref(provider->bus);
        free(provider);
        return r;
    }

    provider->last_register_try = time(NULL);
    register_provider(provider);

    *out = provider;
    return 0;
}

void battery_bluez_set_percent(BatteryBluez *provider, int percent)
{
    if (!provider || percent < 0)
        return;

    if (percent > 100)
        percent = 100;

    if (provider->percent_valid &&
        provider->percent == (unsigned char)percent)
        return;

    provider->percent = (unsigned char)percent;
    provider->percent_valid = 1;

    app_logf("BlueZ Akku: %d %%", percent);

    if (provider->bus) {
        int r = sd_bus_emit_properties_changed(provider->bus,
                                               BATTERY_PATH,
                                               "org.bluez.BatteryProvider1",
                                               "Percentage",
                                               NULL);
        if (r < 0)
            app_logf("BlueZ Akku: PropertiesChanged Fehler %d", r);
    }
}

void battery_bluez_process(BatteryBluez *provider)
{
    if (!provider || !provider->bus)
        return;

    for (;;) {
        int r = sd_bus_process(provider->bus, NULL);
        if (r <= 0)
            break;
    }

    time_t now = time(NULL);
    if (now - provider->last_register_try >= REGISTER_RETRY_SECONDS) {
        provider->last_register_try = now;
        register_provider(provider);
    }
}

void battery_bluez_close(BatteryBluez *provider)
{
    if (!provider)
        return;

    if (provider->bus && provider->registered) {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message *reply = NULL;

        int r = sd_bus_call_method(provider->bus,
                                   BLUEZ_SERVICE,
                                   BLUEZ_ADAPTER_PATH,
                                   "org.bluez.BatteryProviderManager1",
                                   "UnregisterBatteryProvider",
                                   &error,
                                   &reply,
                                   "o",
                                   PROVIDER_PATH);
        if (r < 0 &&
            !sd_bus_error_has_name(&error, "org.bluez.Error.DoesNotExist"))
            app_logf("BlueZ Akku: Abmelden fehlgeschlagen");

        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
    }

    sd_bus_slot_unref(provider->slot);
    sd_bus_unref(provider->bus);
    free(provider);
}
