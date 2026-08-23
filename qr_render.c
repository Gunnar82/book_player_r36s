#include "qr_render.h"
#include <qrencode.h>

int qr_render_url(SDL_Renderer *renderer,const char *url,int x,int y,int pixel_size){
    if(!renderer||!url||!url[0]||pixel_size<64)return -1;

    QRcode *qr=QRcode_encodeString8bit(url,0,QR_ECLEVEL_M);
    if(!qr)return -1;

    const int quiet=4;
    const int modules=qr->width+quiet*2;
    int scale=pixel_size/modules;
    if(scale<1)scale=1;

    int real=modules*scale;

    SDL_Rect bg={x,y,real,real};
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer,&bg);

    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    for(int yy=0;yy<qr->width;yy++){
        for(int xx=0;xx<qr->width;xx++){
            if(!(qr->data[yy*qr->width+xx]&1))continue;
            SDL_Rect r={
                x+(xx+quiet)*scale,
                y+(yy+quiet)*scale,
                scale,
                scale
            };
            SDL_RenderFillRect(renderer,&r);
        }
    }

    QRcode_free(qr);
    return real;
}
