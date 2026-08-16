#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "led.h"
#include "storage.h"

static int led_last_state=-1;
static int warned_once=0;
static int initialized=0;
static int current_gpio=-1;
static int current_manual=0;
static char led_value_path[128]="";

static void update_path(void){
    if(current_gpio>=0)snprintf(led_value_path,sizeof(led_value_path),"/sys/class/gpio/gpio%d/value",current_gpio);
    else led_value_path[0]='\0';
    led_last_state=-1;warned_once=0;
}
int led_gpio_available(int gpio){struct stat st;char path[128];if(gpio<0)return 0;snprintf(path,sizeof(path),"/sys/class/gpio/gpio%d/value",gpio);return stat(path,&st)==0&&S_ISREG(st.st_mode);}
static int read_direction_out(int gpio){char path[128],buf[32];FILE *fp;snprintf(path,sizeof(path),"/sys/class/gpio/gpio%d/direction",gpio);fp=fopen(path,"r");if(!fp)return 0;if(!fgets(buf,sizeof(buf),fp)){fclose(fp);return 0;}fclose(fp);return !strncmp(buf,"out",3);}
static int detect_led_gpio(void){
    DIR *dir=opendir("/sys/class/gpio");struct dirent *entry;int found=-1,count=0;if(!dir)return -1;
    while((entry=readdir(dir))!=NULL){const char *p;int gpio;if(strncmp(entry->d_name,"gpio",4))continue;p=entry->d_name+4;if(!*p||!isdigit((unsigned char)*p))continue;for(const char *q=p;*q;q++)if(!isdigit((unsigned char)*q)){p=NULL;break;}if(!p)continue;gpio=atoi(entry->d_name+4);if(!led_gpio_available(gpio)||!read_direction_out(gpio))continue;found=gpio;if(++count>1)break;}
    closedir(dir);return count==1?found:-1;
}
void led_init(void){int gpio=-1,manual=0;if(initialized)return;initialized=1;if(get_led_gpio_config(&gpio,&manual)>0&&gpio>=0){current_gpio=gpio;current_manual=manual?1:0;update_path();return;}current_gpio=detect_led_gpio();current_manual=0;update_path();if(current_gpio>=0)set_led_gpio_config(current_gpio,0);}
int led_get_gpio(void){led_init();return current_gpio;}
int led_gpio_is_manual(void){led_init();return current_manual;}
int led_set_gpio_manual(int gpio){if(gpio<0||gpio>511)return -1;initialized=1;current_gpio=gpio;current_manual=1;update_path();return set_led_gpio_config(current_gpio,1);}
int led_reset_gpio_auto(void){initialized=1;current_gpio=detect_led_gpio();current_manual=0;update_path();return set_led_gpio_config(current_gpio,0);}
void led_set(int on){FILE *fp;led_init();on=on?1:0;if(current_gpio<0||!led_value_path[0]||on==led_last_state)return;fp=fopen(led_value_path,"w");if(!fp){if(!warned_once){fprintf(stderr,"LED: Kann %s nicht schreiben (Rechte?). Blink-Funktion bleibt ohne Effekt.\n",led_value_path);warned_once=1;}return;}fprintf(fp,"%d",on);fclose(fp);led_last_state=on;}
