#include "bluetooth_agent.h"
#include "app_log.h"

#include <systemd/sd-bus.h>

#define AGENT_PATH "/org/hoerspiel/BluezAgent"

static sd_bus *agent_bus=NULL;
static sd_bus_slot *agent_slot=NULL;
static int agent_registered=0;

static int method_release(sd_bus_message *m,void *userdata,sd_bus_error *ret_error)
{
    (void)userdata;(void)ret_error;
    app_logf("Bluetooth Agent: Release");
    return sd_bus_reply_method_return(m,"");
}

static int method_cancel(sd_bus_message *m,void *userdata,sd_bus_error *ret_error)
{
    (void)userdata;(void)ret_error;
    app_logf("Bluetooth Agent: Cancel");
    return sd_bus_reply_method_return(m,"");
}

static int reject_interactive(sd_bus_message *m,void *userdata,sd_bus_error *ret_error)
{
    (void)m;(void)userdata;
    app_logf("Bluetooth Agent: Interaktive Anfrage vor UI-Unterstuetzung abgelehnt");
    return sd_bus_error_set_const(ret_error,"org.bluez.Error.Rejected","Pairing confirmation UI not implemented yet");
}

static const sd_bus_vtable agent_vtable[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Release","","",method_release,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestPinCode","o","s",reject_interactive,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("DisplayPinCode","os","",reject_interactive,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestPasskey","o","u",reject_interactive,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("DisplayPasskey","ouq","",reject_interactive,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestConfirmation","ou","",reject_interactive,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestAuthorization","o","",reject_interactive,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("AuthorizeService","os","",reject_interactive,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Cancel","","",method_cancel,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

int bluetooth_agent_start(void)
{
    if(agent_registered)return 0;

    int r=sd_bus_open_system(&agent_bus);
    if(r<0){app_logf("Bluetooth Agent: System-D-Bus Fehler %d",r);return -1;}

    r=sd_bus_add_object_vtable(agent_bus,&agent_slot,AGENT_PATH,"org.bluez.Agent1",agent_vtable,NULL);
    if(r<0){app_logf("Bluetooth Agent: Objekt konnte nicht registriert werden (%d)",r);bluetooth_agent_stop();return -1;}

    sd_bus_error error=SD_BUS_ERROR_NULL;
    sd_bus_message *reply=NULL;
    r=sd_bus_call_method(agent_bus,"org.bluez","/org/bluez","org.bluez.AgentManager1","RegisterAgent",&error,&reply,"os",AGENT_PATH,"KeyboardDisplay");
    sd_bus_message_unref(reply);
    if(r<0){
        app_logf("Bluetooth Agent: RegisterAgent fehlgeschlagen (%s)",error.name?error.name:"?");
        sd_bus_error_free(&error);
        bluetooth_agent_stop();
        return -1;
    }
    sd_bus_error_free(&error);

    r=sd_bus_call_method(agent_bus,"org.bluez","/org/bluez","org.bluez.AgentManager1","RequestDefaultAgent",&error,&reply,"o",AGENT_PATH);
    sd_bus_message_unref(reply);
    if(r<0){
        app_logf("Bluetooth Agent: Default-Agent fehlgeschlagen (%s)",error.name?error.name:"?");
        sd_bus_error_free(&error);
        bluetooth_agent_stop();
        return -1;
    }
    sd_bus_error_free(&error);

    agent_registered=1;
    app_logf("Bluetooth Agent: als Default registriert");
    return 0;
}

void bluetooth_agent_process(void)
{
    if(!agent_bus)return;
    for(;;){
        int r=sd_bus_process(agent_bus,NULL);
        if(r<=0)break;
    }
}

void bluetooth_agent_stop(void)
{
    if(agent_bus&&agent_registered){
        sd_bus_error error=SD_BUS_ERROR_NULL;
        sd_bus_message *reply=NULL;
        int r=sd_bus_call_method(agent_bus,"org.bluez","/org/bluez","org.bluez.AgentManager1","UnregisterAgent",&error,&reply,"o",AGENT_PATH);
        if(r<0&&error.name)app_logf("Bluetooth Agent: UnregisterAgent %s",error.name);
        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
    }
    agent_registered=0;
    agent_slot=sd_bus_slot_unref(agent_slot);
    agent_bus=sd_bus_unref(agent_bus);
}

int bluetooth_agent_active(void)
{
    return agent_registered;
}
