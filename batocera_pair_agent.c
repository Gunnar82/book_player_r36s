#include "batocera_pair_agent.h"
#include "app_log.h"

#include <systemd/sd-bus.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define AGENT_PATH "/hoerspiel/agent"
#define AGENT_IFACE "org.bluez.Agent1"

static int g_state=BATOCERA_PAIR_IDLE;
static unsigned int g_passkey;
static int g_answer; /* 0 none, 1 accept, -1 reject */
static int g_cancel;
static int g_pair_done;
static int g_pair_result;

static void set_state(int s){__atomic_store_n(&g_state,s,__ATOMIC_RELEASE);}
BatoceraPairState batocera_pair_agent_state(void){return (BatoceraPairState)__atomic_load_n(&g_state,__ATOMIC_ACQUIRE);}
uint32_t batocera_pair_agent_passkey(void){return __atomic_load_n(&g_passkey,__ATOMIC_ACQUIRE);}
void batocera_pair_agent_respond(int accept){__atomic_store_n(&g_answer,accept?1:-1,__ATOMIC_RELEASE);}
void batocera_pair_agent_cancel(void){__atomic_store_n(&g_cancel,1,__ATOMIC_RELEASE);}

static int agent_release(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;return sd_bus_reply_method_return(m,"");}
static int agent_authorize(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;return sd_bus_reply_method_return(m,"");}
static int agent_authorize_service(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;const char *path=NULL,*uuid=NULL;sd_bus_message_read(m,"os",&path,&uuid);app_logf("BT Pair-Agent: AuthorizeService %s",uuid?uuid:"");return sd_bus_reply_method_return(m,"");}
static int agent_request_pin(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;const char *path=NULL;sd_bus_message_read(m,"o",&path);app_logf("BT Pair-Agent: RequestPinCode -> 0000");return sd_bus_reply_method_return(m,"s","0000");}
static int agent_request_passkey(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;const char *path=NULL;sd_bus_message_read(m,"o",&path);app_logf("BT Pair-Agent: RequestPasskey -> 000000");return sd_bus_reply_method_return(m,"u",0U);}
static int agent_display_passkey(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;const char *path=NULL;uint32_t passkey=0;uint16_t entered=0;sd_bus_message_read(m,"ouq",&path,&passkey,&entered);__atomic_store_n(&g_passkey,passkey,__ATOMIC_RELEASE);app_logf("BT Pair-Agent: DisplayPasskey %06u",passkey);return sd_bus_reply_method_return(m,"");}
static int agent_display_pin(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;const char *path=NULL,*pin=NULL;sd_bus_message_read(m,"os",&path,&pin);app_logf("BT Pair-Agent: DisplayPinCode %s",pin?pin:"");return sd_bus_reply_method_return(m,"");}
static int agent_cancel(sd_bus_message *m,void *userdata,sd_bus_error *ret_error){(void)userdata;(void)ret_error;app_logf("BT Pair-Agent: Cancel");__atomic_store_n(&g_cancel,1,__ATOMIC_RELEASE);return sd_bus_reply_method_return(m,"");}

static int agent_request_confirmation(sd_bus_message *m,void *userdata,sd_bus_error *ret_error)
{
    (void)userdata;(void)ret_error;
    const char *path=NULL;uint32_t passkey=0;
    if(sd_bus_message_read(m,"ou",&path,&passkey)<0)return -EINVAL;
    __atomic_store_n(&g_passkey,passkey,__ATOMIC_RELEASE);
    __atomic_store_n(&g_answer,0,__ATOMIC_RELEASE);
    set_state(BATOCERA_PAIR_CONFIRM);
    app_logf("BT Pair-Agent: bestaetige Passkey %06u",passkey);

    for(int i=0;i<600;i++){
        int answer=__atomic_load_n(&g_answer,__ATOMIC_ACQUIRE);
        if(answer>0){set_state(BATOCERA_PAIR_WORKING);return sd_bus_reply_method_return(m,"");}
        if(answer<0||__atomic_load_n(&g_cancel,__ATOMIC_ACQUIRE)){
            set_state(BATOCERA_PAIR_WORKING);
            return sd_bus_reply_method_errorf(m,"org.bluez.Error.Rejected","Pairing rejected by user");
        }
        usleep(50000);
    }
    set_state(BATOCERA_PAIR_WORKING);
    return sd_bus_reply_method_errorf(m,"org.bluez.Error.Rejected","Pairing confirmation timed out");
}

static const sd_bus_vtable agent_vtable[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Release","","",agent_release,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestPinCode","o","s",agent_request_pin,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestPasskey","o","u",agent_request_passkey,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("DisplayPasskey","ouq","",agent_display_passkey,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("DisplayPinCode","os","",agent_display_pin,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestConfirmation","ou","",agent_request_confirmation,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("RequestAuthorization","o","",agent_authorize,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("AuthorizeService","os","",agent_authorize_service,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Cancel","","",agent_cancel,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

static int pair_reply(sd_bus_message *m,void *userdata,sd_bus_error *ret_error)
{
    (void)userdata;(void)ret_error;
    const sd_bus_error *e=sd_bus_message_get_error(m);
    if(e){
        app_logf("BT Pair-Agent: Pair fehlgeschlagen: %s %s",e->name?e->name:"",e->message?e->message:"");
        g_pair_result=-1;
    }else{
        app_logf("BT Pair-Agent: Pair erfolgreich");
        g_pair_result=0;
    }
    __atomic_store_n(&g_pair_done,1,__ATOMIC_RELEASE);
    return 1;
}

static void mac_to_path(const char *mac,char *path,size_t n)
{
    char id[18];snprintf(id,sizeof(id),"%s",mac?mac:"");
    for(char *p=id;*p;p++)if(*p==':')*p='_';
    snprintf(path,n,"/org/bluez/hci0/dev_%s",id);
}

int batocera_pair_agent_pair(const char *mac)
{
    if(!mac||strlen(mac)!=17)return -1;
    __atomic_store_n(&g_cancel,0,__ATOMIC_RELEASE);
    __atomic_store_n(&g_answer,0,__ATOMIC_RELEASE);
    __atomic_store_n(&g_passkey,0,__ATOMIC_RELEASE);
    __atomic_store_n(&g_pair_done,0,__ATOMIC_RELEASE);
    g_pair_result=-1;
    set_state(BATOCERA_PAIR_WORKING);

    sd_bus *bus=NULL;sd_bus_slot *slot=NULL,*pair_slot=NULL;
    sd_bus_error error=SD_BUS_ERROR_NULL;sd_bus_message *reply=NULL;
    int registered=0;
    int r=sd_bus_open_system(&bus);
    if(r<0)goto fail;
    r=sd_bus_add_object_vtable(bus,&slot,AGENT_PATH,AGENT_IFACE,agent_vtable,NULL);
    if(r<0)goto fail;
    r=sd_bus_call_method(bus,"org.bluez","/org/bluez","org.bluez.AgentManager1","RegisterAgent",&error,&reply,"os",AGENT_PATH,"DisplayYesNo");
    sd_bus_message_unref(reply);reply=NULL;sd_bus_error_free(&error);
    if(r<0){app_logf("BT Pair-Agent: RegisterAgent fehlgeschlagen (%d)",r);goto fail;}
    registered=1;

    char path[96];mac_to_path(mac,path,sizeof(path));
    app_logf("BT Pair-Agent: Pair %s",mac);

    /* Some devices briefly report Connected=true before they are paired.
       Disconnect first so BlueZ can perform the actual authentication. */
    sd_bus_call_method(bus,"org.bluez",path,"org.bluez.Device1","Disconnect",NULL,NULL,"");
    usleep(200000);

    r=sd_bus_call_method_async(bus,&pair_slot,"org.bluez",path,"org.bluez.Device1","Pair",pair_reply,NULL,0,"");
    if(r<0)goto fail;

    for(int ticks=0;ticks<900&&!__atomic_load_n(&g_pair_done,__ATOMIC_ACQUIRE);ticks++){
        if(__atomic_load_n(&g_cancel,__ATOMIC_ACQUIRE)){
            sd_bus_call_method(bus,"org.bluez",path,"org.bluez.Device1","CancelPairing",NULL,NULL,"");
        }
        do{r=sd_bus_process(bus,NULL);}while(r>0);
        if(r<0)break;
        sd_bus_wait(bus,50000);
    }

    if(!__atomic_load_n(&g_pair_done,__ATOMIC_ACQUIRE))g_pair_result=-1;
    if(g_pair_result==0){
        int trusted=1;
        r=sd_bus_set_property(bus,"org.bluez",path,"org.bluez.Device1","Trusted",&error,"b",trusted);
        if(r<0){app_logf("BT Pair-Agent: Trusted setzen fehlgeschlagen");g_pair_result=-1;}
        sd_bus_error_free(&error);
    }

    if(registered)sd_bus_call_method(bus,"org.bluez","/org/bluez","org.bluez.AgentManager1","UnregisterAgent",NULL,NULL,"o",AGENT_PATH);
    sd_bus_slot_unref(pair_slot);sd_bus_slot_unref(slot);sd_bus_unref(bus);
    set_state(g_pair_result==0?BATOCERA_PAIR_SUCCESS:BATOCERA_PAIR_FAILED);
    return g_pair_result;

fail:
    if(error.name||error.message)app_logf("BT Pair-Agent: %s %s",error.name?error.name:"",error.message?error.message:"");
    if(registered&&bus)sd_bus_call_method(bus,"org.bluez","/org/bluez","org.bluez.AgentManager1","UnregisterAgent",NULL,NULL,"o",AGENT_PATH);
    sd_bus_message_unref(reply);sd_bus_error_free(&error);sd_bus_slot_unref(pair_slot);sd_bus_slot_unref(slot);sd_bus_unref(bus);
    set_state(BATOCERA_PAIR_FAILED);
    return -1;
}
