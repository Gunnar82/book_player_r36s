#include "mpris_bridge.h"
#include "media_feedback.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MPRIS_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_NAME "org.mpris.MediaPlayer2.HoerspielPlayer"
#define PLAYER_IFACE "org.mpris.MediaPlayer2.Player"
#define ROOT_IFACE "org.mpris.MediaPlayer2"
#define BLUEZ_ROOT "/com/gunnar/hoerspiel"
#define BLUEZ_PLAYER BLUEZ_ROOT "/player"

static uint64_t monotonic_ms(void)
{
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC,&ts)!=0)return 0;
    return (uint64_t)ts.tv_sec*1000ULL+(uint64_t)ts.tv_nsec/1000000ULL;
}

static void queue_action(MprisBridge *b, MediaKeyAction action)
{
    if(!b||action==MEDIA_KEY_NONE||b->action_count>=16)return;
    b->actions[b->action_count++]=action;
}

static int method_action(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    (void)ret_error;
    MprisBridge *b=(MprisBridge*)userdata;
    const char *member=sd_bus_message_get_member(m);
    MediaKeyAction action=MEDIA_KEY_NONE;
    if(!strcmp(member,"Next"))action=MEDIA_KEY_NEXT;
    else if(!strcmp(member,"Previous"))action=MEDIA_KEY_PREVIOUS;
    else if(!strcmp(member,"Pause"))action=MEDIA_KEY_PAUSE;
    else if(!strcmp(member,"PlayPause"))action=MEDIA_KEY_PLAY_PAUSE;
    else if(!strcmp(member,"Stop"))action=MEDIA_KEY_STOP;
    else if(!strcmp(member,"Play"))action=MEDIA_KEY_PLAY;
    if(action!=MEDIA_KEY_NONE){queue_action(b,action);media_feedback_show(action,-1,member);}
    return sd_bus_reply_method_return(m,"");
}

static int method_noop(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    (void)userdata;(void)ret_error;
    return sd_bus_reply_method_return(m,"");
}

static int prop_string(sd_bus *bus,const char *path,const char *interface,const char *property,
                       sd_bus_message *reply,void *userdata,sd_bus_error *ret_error)
{
    (void)bus;(void)path;(void)interface;(void)ret_error;
    MprisBridge *b=(MprisBridge*)userdata;
    if(!strcmp(property,"PlaybackStatus"))return sd_bus_message_append(reply,"s",b->playback_status);
    if(!strcmp(property,"LoopStatus"))return sd_bus_message_append(reply,"s","None");
    if(!strcmp(property,"Identity"))return sd_bus_message_append(reply,"s","Hoerspiel Player");
    return sd_bus_message_append(reply,"s","");
}

static int prop_bool(sd_bus *bus,const char *path,const char *interface,const char *property,
                     sd_bus_message *reply,void *userdata,sd_bus_error *ret_error)
{
    (void)bus;(void)path;(void)interface;(void)userdata;(void)ret_error;
    int v=0;
    if(!strcmp(property,"CanGoNext")||!strcmp(property,"CanGoPrevious")||
       !strcmp(property,"CanPlay")||!strcmp(property,"CanPause")||
       !strcmp(property,"CanControl"))v=1;
    return sd_bus_message_append(reply,"b",v);
}

static int prop_double(sd_bus *bus,const char *path,const char *interface,const char *property,
                       sd_bus_message *reply,void *userdata,sd_bus_error *ret_error)
{
    (void)bus;(void)path;(void)interface;(void)ret_error;
    MprisBridge *b=(MprisBridge*)userdata;
    double v=1.0;
    if(!strcmp(property,"Volume"))v=b->volume;
    return sd_bus_message_append(reply,"d",v);
}

static int prop_position(sd_bus *bus,const char *path,const char *interface,const char *property,
                         sd_bus_message *reply,void *userdata,sd_bus_error *ret_error)
{
    (void)bus;(void)path;(void)interface;(void)property;(void)ret_error;
    MprisBridge *b=(MprisBridge*)userdata;
    return sd_bus_message_append(reply,"x",b->position_us);
}

static int append_metadata(sd_bus_message *m,MprisBridge *b)
{
    char track_path[96];
    snprintf(track_path,sizeof(track_path),"/org/mpris/MediaPlayer2/Track/%d",b->track_number>0?b->track_number:1);
    int r=sd_bus_message_open_container(m,'a',"{sv}");if(r<0)return r;
#define ADD_S(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"s");if(r<0)return r;r=sd_bus_message_append(m,"s",VAL);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
#define ADD_X(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"x");if(r<0)return r;r=sd_bus_message_append(m,"x",(int64_t)(VAL));if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
#define ADD_I(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"i");if(r<0)return r;r=sd_bus_message_append(m,"i",(int32_t)(VAL));if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
#define ADD_O(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"o");if(r<0)return r;r=sd_bus_message_append(m,"o",VAL);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
    ADD_O("mpris:trackid",track_path);
    ADD_S("xesam:title",b->title[0]?b->title:"--");
    ADD_S("xesam:album",b->album[0]?b->album:"--");
    ADD_X("mpris:length",b->length_us);
    ADD_I("xesam:trackNumber",b->track_number);
#undef ADD_S
#undef ADD_X
#undef ADD_I
#undef ADD_O
    return sd_bus_message_close_container(m);
}

static int prop_metadata(sd_bus *bus,const char *path,const char *interface,const char *property,
                         sd_bus_message *reply,void *userdata,sd_bus_error *ret_error)
{
    (void)bus;(void)path;(void)interface;(void)property;(void)ret_error;
    return append_metadata(reply,(MprisBridge*)userdata);
}

static int prop_empty_strings(sd_bus *bus,const char *path,const char *interface,const char *property,
                              sd_bus_message *reply,void *userdata,sd_bus_error *ret_error)
{
    (void)bus;(void)path;(void)interface;(void)property;(void)userdata;(void)ret_error;
    int r=sd_bus_message_open_container(reply,'a',"s");if(r<0)return r;
    return sd_bus_message_close_container(reply);
}

static const sd_bus_vtable root_vtable[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Raise","","",method_noop,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Quit","","",method_noop,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_PROPERTY("CanQuit","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("CanRaise","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("HasTrackList","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("Identity","s",prop_string,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("SupportedUriSchemes","as",prop_empty_strings,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("SupportedMimeTypes","as",prop_empty_strings,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_VTABLE_END
};

static const sd_bus_vtable player_vtable[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Next","","",method_action,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Previous","","",method_action,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Pause","","",method_action,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("PlayPause","","",method_action,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Stop","","",method_action,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Play","","",method_action,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_PROPERTY("PlaybackStatus","s",prop_string,0,SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("LoopStatus","s",prop_string,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("Rate","d",prop_double,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("Metadata","a{sv}",prop_metadata,0,SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Volume","d",prop_double,0,SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Position","x",prop_position,0,0),
    SD_BUS_PROPERTY("MinimumRate","d",prop_double,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("MaximumRate","d",prop_double,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("CanGoNext","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("CanGoPrevious","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("CanPlay","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("CanPause","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("CanSeek","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_PROPERTY("CanControl","b",prop_bool,0,SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_VTABLE_END
};

static int append_player_properties(sd_bus_message *m,MprisBridge *b)
{
    int r;
#define P_S(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"s");if(r<0)return r;r=sd_bus_message_append(m,"s",VAL);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
#define P_B(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"b");if(r<0)return r;r=sd_bus_message_append(m,"b",VAL);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
#define P_D(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"d");if(r<0)return r;r=sd_bus_message_append(m,"d",VAL);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
#define P_X(KEY,VAL) do{r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;r=sd_bus_message_append(m,"s",KEY);if(r<0)return r;r=sd_bus_message_open_container(m,'v',"x");if(r<0)return r;r=sd_bus_message_append(m,"x",(int64_t)(VAL));if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;r=sd_bus_message_close_container(m);if(r<0)return r;}while(0)
    P_S("PlaybackStatus",b->playback_status);
    P_S("Identity","Hoerspiel Player");
    P_S("LoopStatus","None");
    P_D("Rate",1.0);
    P_D("Volume",b->volume);
    P_X("Position",b->position_us);
    P_D("MinimumRate",1.0);
    P_D("MaximumRate",1.0);
    P_B("CanGoNext",1);P_B("CanGoPrevious",1);P_B("CanPlay",1);P_B("CanPause",1);P_B("CanSeek",0);P_B("CanControl",1);
    r=sd_bus_message_open_container(m,'e',"sv");if(r<0)return r;
    r=sd_bus_message_append(m,"s","Metadata");if(r<0)return r;
    r=sd_bus_message_open_container(m,'v',"a{sv}");if(r<0)return r;
    r=append_metadata(m,b);if(r<0)return r;
    r=sd_bus_message_close_container(m);if(r<0)return r;
    r=sd_bus_message_close_container(m);if(r<0)return r;
#undef P_S
#undef P_B
#undef P_D
#undef P_X
    return 0;
}

static int object_manager_get(sd_bus_message *m,void *userdata,sd_bus_error *ret_error)
{
    (void)ret_error;
    MprisBridge *b=(MprisBridge*)userdata;
    sd_bus_message *reply=NULL;
    int r=sd_bus_message_new_method_return(m,&reply);if(r<0)return r;
    r=sd_bus_message_open_container(reply,'a',"{oa{sa{sv}}}");if(r<0)goto out;
    r=sd_bus_message_open_container(reply,'e',"oa{sa{sv}}");if(r<0)goto out;
    r=sd_bus_message_append(reply,"o",BLUEZ_PLAYER);if(r<0)goto out;
    r=sd_bus_message_open_container(reply,'a',"{sa{sv}}");if(r<0)goto out;
    r=sd_bus_message_open_container(reply,'e',"sa{sv}");if(r<0)goto out;
    r=sd_bus_message_append(reply,"s",PLAYER_IFACE);if(r<0)goto out;
    r=sd_bus_message_open_container(reply,'a',"{sv}");if(r<0)goto out;
    r=append_player_properties(reply,b);if(r<0)goto out;
    r=sd_bus_message_close_container(reply);if(r<0)goto out;
    r=sd_bus_message_close_container(reply);if(r<0)goto out;
    r=sd_bus_message_close_container(reply);if(r<0)goto out;
    r=sd_bus_message_close_container(reply);if(r<0)goto out;
    r=sd_bus_message_close_container(reply);if(r<0)goto out;
    r=sd_bus_send(NULL,reply,NULL);
out:
    sd_bus_message_unref(reply);
    return r;
}

static const sd_bus_vtable object_manager_vtable[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("GetManagedObjects","","a{oa{sa{sv}}}",object_manager_get,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

static int register_bluez(MprisBridge *b)
{
    if(!b->system_bus||b->bluez_registered)return 0;
    uint64_t now=monotonic_ms();
    if(b->last_bluez_try_ms&&now-b->last_bluez_try_ms<2000)return 0;
    b->last_bluez_try_ms=now;
    for(int i=0;i<8;i++){
        char adapter[64];snprintf(adapter,sizeof(adapter),"/org/bluez/hci%d",i);
        sd_bus_message *m=NULL,*reply=NULL;
        sd_bus_error error=SD_BUS_ERROR_NULL;
        int r=sd_bus_message_new_method_call(b->system_bus,&m,"org.bluez",adapter,"org.bluez.Media1","RegisterApplication");
        if(r<0)continue;
        r=sd_bus_message_append(m,"o",BLUEZ_ROOT);
        if(r>=0)r=sd_bus_message_open_container(m,'a',"{sv}");
        if(r>=0)r=sd_bus_message_close_container(m);
        if(r>=0)r=sd_bus_call(b->system_bus,m,1000000,&error,&reply);
        sd_bus_error_free(&error);sd_bus_message_unref(reply);sd_bus_message_unref(m);
        if(r>=0){b->bluez_registered=1;fprintf(stderr,"MPRIS/BlueZ: Media-Anwendung auf hci%d registriert.\n",i);return 1;}
    }
    return 0;
}

void mpris_bridge_init(MprisBridge *b)
{
    if(!b)return;memset(b,0,sizeof(*b));
    snprintf(b->playback_status,sizeof(b->playback_status),"Stopped");b->volume=1.0;
    if(sd_bus_default_user(&b->session_bus)>=0){
        int r=sd_bus_add_object_vtable(b->session_bus,&b->session_root_slot,MPRIS_PATH,ROOT_IFACE,root_vtable,b);
        if(r>=0)r=sd_bus_add_object_vtable(b->session_bus,&b->session_player_slot,MPRIS_PATH,PLAYER_IFACE,player_vtable,b);
        if(r>=0)r=sd_bus_request_name(b->session_bus,MPRIS_NAME,0);
        if(r<0){fprintf(stderr,"MPRIS: Session-Bus nicht nutzbar (%d).\n",r);sd_bus_flush_close_unref(b->session_bus);b->session_bus=NULL;}
        else fprintf(stderr,"MPRIS: %s bereit.\n",MPRIS_NAME);
    }
    if(sd_bus_default_system(&b->system_bus)>=0){
        int r=sd_bus_add_object_vtable(b->system_bus,&b->bluez_manager_slot,BLUEZ_ROOT,"org.freedesktop.DBus.ObjectManager",object_manager_vtable,b);
        if(r>=0)r=sd_bus_add_object_vtable(b->system_bus,&b->bluez_player_slot,BLUEZ_PLAYER,PLAYER_IFACE,player_vtable,b);
        if(r<0){fprintf(stderr,"MPRIS/BlueZ: System-Bus-Export fehlgeschlagen (%d).\n",r);sd_bus_flush_close_unref(b->system_bus);b->system_bus=NULL;}
    }
}

void mpris_bridge_close(MprisBridge *b)
{
    if(!b)return;
    b->session_root_slot=sd_bus_slot_unref(b->session_root_slot);b->session_player_slot=sd_bus_slot_unref(b->session_player_slot);
    b->bluez_manager_slot=sd_bus_slot_unref(b->bluez_manager_slot);b->bluez_player_slot=sd_bus_slot_unref(b->bluez_player_slot);
    b->session_bus=sd_bus_flush_close_unref(b->session_bus);b->system_bus=sd_bus_flush_close_unref(b->system_bus);
}

void mpris_bridge_update(MprisBridge *b,const char *album,const char *title,int track_number,int track_count,
                         double duration_seconds,double position_seconds,int has_music,int paused,int playing,double volume)
{
    if(!b)return;
    char old_status[16],old_title[256],old_album[256];double old_volume=b->volume;
    snprintf(old_status,sizeof(old_status),"%s",b->playback_status);snprintf(old_title,sizeof(old_title),"%s",b->title);snprintf(old_album,sizeof(old_album),"%s",b->album);
    snprintf(b->album,sizeof(b->album),"%s",album?album:"");snprintf(b->title,sizeof(b->title),"%s",title?title:"");
    b->track_number=track_number;b->track_count=track_count;
    b->length_us=duration_seconds>0?(int64_t)(duration_seconds*1000000.0):0;
    b->position_us=position_seconds>0?(int64_t)(position_seconds*1000000.0):0;
    b->volume=volume<0?0:(volume>1?1:volume);
    snprintf(b->playback_status,sizeof(b->playback_status),"%s",!has_music?"Stopped":(paused||!playing?"Paused":"Playing"));
    int meta_changed=strcmp(old_title,b->title)||strcmp(old_album,b->album);
    int status_changed=strcmp(old_status,b->playback_status);
    int volume_changed=(old_volume!=b->volume);
    if(b->session_bus&&(meta_changed||status_changed||volume_changed))
        sd_bus_emit_properties_changed(b->session_bus,MPRIS_PATH,PLAYER_IFACE,"PlaybackStatus","Metadata","Volume",NULL);
    if(b->system_bus&&(meta_changed||status_changed||volume_changed))
        sd_bus_emit_properties_changed(b->system_bus,BLUEZ_PLAYER,PLAYER_IFACE,"PlaybackStatus","Metadata","Volume",NULL);
}

int mpris_bridge_poll(MprisBridge *b,MediaKeyAction *actions,int max_actions)
{
    if(!b||!actions||max_actions<=0)return 0;
    if(b->session_bus)while(sd_bus_process(b->session_bus,NULL)>0){}
    if(b->system_bus){while(sd_bus_process(b->system_bus,NULL)>0){}register_bluez(b);}
    int n=b->action_count;if(n>max_actions)n=max_actions;
    for(int i=0;i<n;i++)actions[i]=b->actions[i];
    if(n<b->action_count)memmove(b->actions,b->actions+n,(size_t)(b->action_count-n)*sizeof(b->actions[0]));
    b->action_count-=n;
    return n;
}

int mpris_bridge_bluez_registered(const MprisBridge *b){return b&&b->bluez_registered;}
