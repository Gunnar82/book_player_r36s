#include "batocera_bluetooth.h"
#include "bluetooth.h"
#include "app_log.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int __wrap_bluetooth_scan_paired_trusted(BluetoothDevice *devices,int max_devices)
{
    return batocera_bluetooth_list(devices,max_devices);
}

int __wrap_bluetooth_connect_device(const char *mac)
{
    int rc=batocera_bluetooth_connect(mac);
    app_logf("Bluetooth Batocera: connect %s -> %s",mac?mac:"",rc==0?"OK":"Fehler");
    return rc;
}

void __wrap_bluetooth_autoconnect_start(void)
{
    if(!bluetooth_adapter_powered())return;
    if(!bluetooth_autoconnect||!bluetooth_device_mac[0])return;

    pid_t pid=fork();
    if(pid<0){app_logf("Bluetooth Autoconnect Batocera: fork fehlgeschlagen");return;}
    if(pid==0){
        pid_t child=fork();
        if(child<0)_exit(1);
        if(child>0)_exit(0);
        sleep(2);
        _exit(batocera_bluetooth_connect(bluetooth_device_mac)==0?0:1);
    }
    waitpid(pid,NULL,0);
    app_logf("Bluetooth Autoconnect Batocera: %s",bluetooth_device_mac);
}
