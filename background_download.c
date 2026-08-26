#include "background_download.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

static SDL_mutex *job_mutex;
static SDL_Thread *job_thread;
static BackgroundDownloadStatus job;
static RemoteSelection job_selection[BACKGROUND_DOWNLOAD_MAX_SELECTIONS];
static int job_selection_count;
static int cancel_requested;
static Uint32 rate_sample_time;
static long long rate_sample_bytes;

/* remote_download_selection is wrapped by background_download_ui.c for calls
 * from the synchronous download screen. The worker must call the real
 * implementation or it would recursively start another worker. */
int __real_remote_download_selection(const RemoteSelection *selection,
                                     int selection_count,
                                     DownloadProgressFn progress,
                                     void *userdata,
                                     char *error,
                                     size_t error_size);

static int ensure_mutex(void)
{
    if(job_mutex)return 0;
    job_mutex=SDL_CreateMutex();
    return job_mutex?0:-1;
}

static int progress_cb(const char *folder,const char *name,
                       int file_index,int file_count,
                       long long file_now,long long file_total,
                       long long total_now,long long total_size,
                       void *userdata)
{
    (void)userdata;
    Uint32 now=SDL_GetTicks();
    SDL_LockMutex(job_mutex);
    job.file_index=file_index;
    job.file_count=file_count;
    job.file_now=file_now;
    job.file_total=file_total;
    job.total_now=total_now;
    job.total_size=total_size;
    snprintf(job.folder,sizeof(job.folder),"%s",folder?folder:"");
    snprintf(job.filename,sizeof(job.filename),"%s",name?name:"");

    /* Measure against the cumulative download byte counter.  Unlike file_now,
       total_now does not jump back to zero when the next file starts, so the
       displayed rate remains useful across file boundaries. */
    if(rate_sample_time==0){
        rate_sample_time=now;
        rate_sample_bytes=total_now;
    }else if(now-rate_sample_time>=500U){
        if(now>rate_sample_time&&total_now>=rate_sample_bytes){
            double instant=(double)(total_now-rate_sample_bytes)*1000.0/
                           (double)(now-rate_sample_time);
            if(instant>0.0)
                job.rate_bps=job.rate_bps>0.0?job.rate_bps*0.65+instant*0.35:instant;
        }
        rate_sample_time=now;
        rate_sample_bytes=total_now;
    }
    int cancel=cancel_requested;
    SDL_UnlockMutex(job_mutex);
    return cancel?1:0;
}

static int worker(void *unused)
{
    (void)unused;
    char error[256]="";
    int rc=__real_remote_download_selection(job_selection,job_selection_count,
                                            progress_cb,NULL,error,sizeof(error));

    SDL_LockMutex(job_mutex);
    job.result=rc;
    job.active=0;
    job.finished=1;
    job.cancelled=cancel_requested?1:0;
    snprintf(job.error,sizeof(job.error),"%s",error);
    SDL_UnlockMutex(job_mutex);
    return 0;
}

int background_download_start(const RemoteSelection *selection,
                              int selection_count,
                              char *error,size_t error_size)
{
    if(!selection||selection_count<=0||selection_count>BACKGROUND_DOWNLOAD_MAX_SELECTIONS){
        if(error&&error_size)snprintf(error,error_size,"Ungueltige Download-Auswahl");
        return -1;
    }
    if(ensure_mutex()!=0){
        if(error&&error_size)snprintf(error,error_size,"Download-Mutex konnte nicht erstellt werden");
        return -1;
    }

    if(job_thread&&!job.active){
        SDL_WaitThread(job_thread,NULL);
        job_thread=NULL;
    }

    SDL_LockMutex(job_mutex);
    if(job.active){
        SDL_UnlockMutex(job_mutex);
        if(error&&error_size)snprintf(error,error_size,"Ein Download laeuft bereits");
        return -1;
    }
    memcpy(job_selection,selection,(size_t)selection_count*sizeof(job_selection[0]));
    job_selection_count=selection_count;
    memset(&job,0,sizeof(job));
    job.active=1;
    cancel_requested=0;
    rate_sample_time=0;
    rate_sample_bytes=0;
    SDL_UnlockMutex(job_mutex);

    job_thread=SDL_CreateThread(worker,"background-download",NULL);
    if(!job_thread){
        SDL_LockMutex(job_mutex);
        job.active=0;
        job.finished=1;
        job.result=-1;
        snprintf(job.error,sizeof(job.error),"Download-Thread konnte nicht gestartet werden: %s",SDL_GetError());
        SDL_UnlockMutex(job_mutex);
        if(error&&error_size)snprintf(error,error_size,"Download-Thread konnte nicht gestartet werden");
        return -1;
    }
    return 0;
}

void background_download_get_status(BackgroundDownloadStatus *status)
{
    if(!status)return;
    memset(status,0,sizeof(*status));
    if(ensure_mutex()!=0)return;
    SDL_LockMutex(job_mutex);
    *status=job;
    SDL_UnlockMutex(job_mutex);
}

void background_download_cancel(void)
{
    if(ensure_mutex()!=0)return;
    SDL_LockMutex(job_mutex);
    if(job.active)cancel_requested=1;
    SDL_UnlockMutex(job_mutex);
}

void background_download_wait(void)
{
    if(job_thread){
        SDL_WaitThread(job_thread,NULL);
        job_thread=NULL;
    }
}

void background_download_shutdown(void)
{
    if(!job_mutex)return;
    background_download_cancel();
    background_download_wait();
    SDL_DestroyMutex(job_mutex);
    job_mutex=NULL;
}
