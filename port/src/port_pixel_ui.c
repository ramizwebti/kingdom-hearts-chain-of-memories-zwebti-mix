/* Original load-menu backdrop, confirm-window tiles and cartridge font. */
#include <SDL3/SDL.h>
#include <string.h>
#include "port/port_pixel_ui.h"
#include "port/port_ui.h"
#include "sprites_map.h"
#include "sprite_palettes.h"

typedef struct PixelTexture {
    SDL_Texture* texture;
    SDL_Renderer* renderer;
    int width, height;
} PixelTexture;
static PixelTexture sTextures[PORT_PIXEL_LAYER_COUNT];

uint32_t PortPixelColor(unsigned value) {
    unsigned r=value&31, g=(value>>5)&31, b=(value>>10)&31;
    return 0xFF000000u | ((r<<3 | r>>2)<<16) | ((g<<3 | g>>2)<<8) | (b<<3 | b>>2);
}

int PortPixelScale(int width,int height) {
    int scale=SDL_min(width/240,height/160);
    return scale>0 ? scale : 1;
}

void PortPixelRect(PortPixelCanvas* c,int x,int y,int width,int height,uint32_t color) {
    int xx,yy,left=SDL_max(0,x),top=SDL_max(0,y);
    int right=SDL_min(c->width,x+width),bottom=SDL_min(c->height,y+height);
    for(yy=top;yy<bottom;++yy) for(xx=left;xx<right;++xx) c->pixels[yy*c->width+xx]=color;
}

void PortPixelText(PortPixelCanvas* c,int x,int y,int width,const char* text,enum PortPixelInk ink) {
    uint32_t pixels[320*14],face,shadow;
    const u16* palette=ink==PORT_PIXEL_CYAN ? gTextCyanPalette :
        ink==PORT_PIXEL_YELLOW ? gTextYellowPalette :
        ink==PORT_PIXEL_GRAY ? gTextGrayPalette : gTextWhitePalette;
    int xx,yy,w=SDL_min(width,320);
    if(w<=0 || !text) return;
    face=PortPixelColor(palette[1]);shadow=PortPixelColor(palette[2]);
    /* This API rasterizes actual cartridge glyphs into the caller's buffer.
     * Restore their original shadow instead of the pale options-menu shadow. */
    /* The line API reserves space for an ellipsis at its buffer's right edge.
     * Give it the complete scratch line, then clip at the caller's boundary;
     * narrow fields such as "MP" and a two-digit spell cost stay intact. */
    PortUiRenderTextLine(pixels,320,14,text,face);
    for(yy=0;yy<14;++yy) for(xx=0;xx<w;++xx) {
        uint32_t color=pixels[yy*320+xx];
        if(color && x+xx>=0 && x+xx<c->width && y+yy>=0 && y+yy<c->height)
            c->pixels[(y+yy)*c->width+x+xx]=color==face ? face : shadow;
    }
}

void PortPixelFrame(PortPixelCanvas* c,int x,int y,int width,int height) {
    int xx,yy;
    if(width<16 || height<16) return;
    /* Confirm-window tiles have six transparent rows above/below the visible
     * rim and one transparent side column. Specify the visible window bounds,
     * then crop that tile padding instead of compressing the text interior. */
    x-=1;y-=6;width+=2;height+=12;
    for(yy=0;yy<height;++yy) for(xx=0;xx<width;++xx) {
        int col=xx<8 ? 0 : xx>=width-8 ? 2 : 1;
        int row=yy<8 ? 0 : yy>=height-8 ? 2 : 1;
        int tx=col==2 ? xx-(width-8) : xx%8;
        int ty=row==2 ? yy-(height-8) : yy%8;
        int tile=1+row*3+col;
        unsigned value=gConfirmWinTiles[tile*32+ty*4+tx/2];
        unsigned color=(value>>((tx&1)*4))&15;
        if(color && x+xx>=0 && x+xx<c->width && y+yy>=0 && y+yy<c->height)
            c->pixels[(y+yy)*c->width+x+xx]=PortPixelColor(gCard00Palette[color]);
    }
}

void PortPixelBackdrop(PortPixelCanvas* c) {
    int x,y;
    /* Only the visible 240x160 tilemap contains scene artwork. The remaining
     * tilemap area is a bright key color. Repeat the authored scene sideways
     * and extend its sky/floor on taller native canvases. */
    for(y=0;y<c->height;++y) for(x=0;x<c->width;++x) {
        int bx=(x-(c->width-240)/2)%240;
        int by=SDL_clamp(y-(c->height-160)/2,0,159);
        unsigned entry,tx,ty,value,color;
        if(bx<0) bx+=240;
        entry=gMenuLoadBackdropMap[(by/8)*32+bx/8];
        tx=bx&7;ty=by&7;
        if(entry&0x400) tx=7-tx;
        if(entry&0x800) ty=7-ty;
        value=gMenuLoadBgTiles[(entry&1023)*32+ty*4+tx/2];
        color=(value>>((tx&1)*4))&15;
        c->pixels[y*c->width+x]=PortPixelColor(gLoadMenuBgPalettes[(entry>>12)*16+color]);
    }
}

void PortPixelUiReset(void) {
    unsigned i;
    for(i=0;i<PORT_PIXEL_LAYER_COUNT;++i) {
        if(sTextures[i].texture) SDL_DestroyTexture(sTextures[i].texture);
        memset(&sTextures[i],0,sizeof(sTextures[i]));
    }
}

void PortPixelPresent(SDL_Renderer* renderer,enum PortPixelLayer layer,
                      const PortPixelCanvas* c,int x,int y,int scale) {
    PixelTexture* cache;
    SDL_FRect dst;
    if(!renderer || layer>=PORT_PIXEL_LAYER_COUNT || scale<1) return;
    cache=&sTextures[layer];
    if(cache->texture && (cache->renderer!=renderer || cache->width!=c->width || cache->height!=c->height)) {
        SDL_DestroyTexture(cache->texture);cache->texture=NULL;
    }
    if(!cache->texture) {
        cache->texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,c->width,c->height);
        if(!cache->texture) return;
        cache->renderer=renderer;cache->width=c->width;cache->height=c->height;
        SDL_SetTextureScaleMode(cache->texture,SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(cache->texture,SDL_BLENDMODE_BLEND);
    }
    if(!SDL_UpdateTexture(cache->texture,NULL,c->pixels,c->width*sizeof(*c->pixels))) return;
    dst.x=(float)x;dst.y=(float)y;dst.w=(float)(c->width*scale);dst.h=(float)(c->height*scale);
    SDL_RenderTexture(renderer,cache->texture,NULL,&dst);
}
