#include "media_feedback.h"
#include "ui.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

#define FEEDBACK_MS 3000U

static MediaKeyAction current_action=MEDIA_KEY_NONE;
static int current_keycode=-1;
static int current_valid=0;
static char current_source[48]="";
static Uint32 visible_until=0;

static const char *action_symbol(MediaKeyAction action)
{
    switch(action){
        case MEDIA_KEY_PREVIOUS:return "|<<";
        case MEDIA_KEY_NEXT:return ">>|";
        case MEDIA_KEY_PLAY_PAUSE:return "> ||";
        case MEDIA_KEY_PLAY:return ">";
        case MEDIA_KEY_PAUSE:return "||";
        case MEDIA_KEY_STOP:return "[]";
        case MEDIA_KEY_DISPLAY_TOGGLE:return "LCD";
        case MEDIA_KEY_NONE:return "?";
        default:return "?";
    }
}

static const char *action_name(MediaKeyAction action)
{
    switch(action){
        case MEDIA_KEY_PREVIOUS:return "PREVIOUS";
        case MEDIA_KEY_NEXT:return "NEXT";
        case MEDIA_KEY_PLAY_PAUSE:return "PLAY / PAUSE";
        case MEDIA_KEY_PLAY:return "PLAY";
        case MEDIA_KEY_PAUSE:return "PAUSE";
        case MEDIA_KEY_STOP:return "STOP";
        case MEDIA_KEY_DISPLAY_TOGGLE:return "DISPLAY";
        case MEDIA_KEY_NONE:return "UNBEKANNT";
        default:return "UNBEKANNT";
    }
}

void media_feedback_show(MediaKeyAction action,int keycode,const char *source)
{
    /* DISPLAY bleibt absichtlich ohne Overlay, damit ein Display-Toggle
       nicht sofort wieder durch eine sichtbare Rueckmeldung konterkariert wird. */
    if(action==MEDIA_KEY_DISPLAY_TOGGLE)return;

    current_action=action;
    current_keycode=keycode;
    current_valid=1;
    snprintf(current_source,sizeof(current_source),"%s",source?source:"");
    visible_until=SDL_GetTicks()+FEEDBACK_MS;
}

void media_feedback_render(SDL_Renderer *renderer,TTF_Font *font,SDL_Color fg,SDL_Color dim)
{
    if(!renderer||!font||!current_valid)return;
    Uint32 now=SDL_GetTicks();
    if((Sint32)(visible_until-now)<=0){current_valid=0;return;}

    SDL_Rect panel={145,90,350,285};
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,0,0,0,220);
    SDL_RenderFillRect(renderer,&panel);
    SDL_SetRenderDrawColor(renderer,120,120,120,255);
    SDL_RenderDrawRect(renderer,&panel);

    const char *symbol=action_symbol(current_action);
    SDL_Surface *surface=TTF_RenderUTF8_Blended(font,symbol,fg);
    if(surface){
        SDL_Texture *texture=SDL_CreateTextureFromSurface(renderer,surface);
        if(texture){
            int target_h=105;
            int target_w=surface->h>0?(surface->w*target_h)/surface->h:160;
            if(target_w>270)target_w=270;
            SDL_Rect dst={(SCREEN_W-target_w)/2,115,target_w,target_h};
            SDL_RenderCopy(renderer,texture,NULL,&dst);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }

    char line[128];
    int tw=0,th=0;
    snprintf(line,sizeof(line),"%s",action_name(current_action));
    TTF_SizeUTF8(font,line,&tw,&th);
    draw_text(renderer,font,line,(SCREEN_W-tw)/2,235,fg);

    if(current_keycode>=0)
        snprintf(line,sizeof(line),"Keycode: %d",current_keycode);
    else
        snprintf(line,sizeof(line),"Keycode: --");
    TTF_SizeUTF8(font,line,&tw,&th);
    draw_text(renderer,font,line,(SCREEN_W-tw)/2,275,dim);

    if(current_source[0]){
        if(current_keycode<0)
            snprintf(line,sizeof(line),"MPRIS: %s",current_source);
        else
            snprintf(line,sizeof(line),"Quelle: %s",current_source);
        TTF_SizeUTF8(font,line,&tw,&th);
        draw_text(renderer,font,line,(SCREEN_W-tw)/2,310,dim);
    }
}
