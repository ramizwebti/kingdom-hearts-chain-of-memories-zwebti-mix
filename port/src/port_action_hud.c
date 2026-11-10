/* Compact bottom-right status beside the unchanged native Sora portrait. */
#include <SDL3/SDL.h>
#include <string.h>
#include "port/port_action_hud.h"

enum { HUD_WIDTH=70, HUD_HEIGHT=34, GAUGE_WIDTH=54,
       ASSIST_MAX=PORT_ACTION_MAX_ASSIST, ASSIST_COUNT=PORT_ACTION_ASSIST_FAMILY_COUNT };

/* Seven deliberately small, distinct silhouettes; no button or command text. */
static const uint8_t sMagicGlyphs[PORT_ACTION_MAGIC_COUNT][7]={
    {0x08,0x18,0x1c,0x3e,0x3e,0x3e,0x1c}, /* Fire */
    {0x08,0x2a,0x1c,0x7f,0x1c,0x2a,0x08}, /* Blizzard */
    {0x0c,0x18,0x38,0x7e,0x0e,0x0c,0x18}, /* Thunder */
    {0x00,0x1c,0x1c,0x7f,0x1c,0x1c,0x00}, /* Cure */
    {0x1c,0x22,0x49,0x7f,0x49,0x22,0x1c}, /* Gravity */
    {0x3e,0x22,0x14,0x08,0x14,0x22,0x3e}, /* Stop */
    {0x0e,0x11,0x02,0x7c,0x20,0x44,0x38}  /* Aero */
};
static const uint32_t sMagicColors[PORT_ACTION_MAGIC_COUNT]={
    0xffff9254u,0xff98e4ffu,0xffffdf65u,0xff8ce89au,
    0xffb69ae8u,0xfff0ddb2u,0xff9ee5d6u
};
static const char* sAssistNames[ASSIST_COUNT]={
    "SIMBA","GENIE","BAMBI","DUMBO","TINKER BELL","MUSHU","CLOUD",
    "DONALD","GOOFY","ALADDIN","ARIEL","JACK","PETER PAN","BEAST"
};
/* Small cartridge-pixel lettering gives every assist an identifiable name
 * without introducing a command panel, level duplicates or button labels. */
static const uint8_t sLetter[26][5]={
    {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},
    {7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},
    {5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},
    {6,5,6,4,4},{2,5,5,3,1},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},
    {5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},
    {7,1,2,4,7}
};

static void Rect(uint32_t* pixels,int x,int y,int w,int h,uint32_t color) {
    int xx,yy;
    for(yy=y;yy<y+h;++yy) for(xx=x;xx<x+w;++xx)
        if(xx>=0 && xx<HUD_WIDTH && yy>=0 && yy<HUD_HEIGHT)
            pixels[yy*HUD_WIDTH+xx]=color;
}

static void Gauge(uint32_t* pixels,int y,unsigned fill,int health) {
    Rect(pixels,1,y+1,56,7,0xff171a22u);
    Rect(pixels,0,y+2,58,5,0xff171a22u);
    Rect(pixels,2,y+1,54,1,0xffd9d9cbu);
    Rect(pixels,2,y+7,54,1,0xff909aabu);
    Rect(pixels,1,y+2,1,5,0xffb9c2cau);
    Rect(pixels,56,y+2,1,5,0xffb9c2cau);
    Rect(pixels,2,y+2,54,5,0xff172535u);
    Rect(pixels,2,y+2,(int)fill,1,health ? 0xffb7f59au : 0xffa5ddffu);
    Rect(pixels,2,y+3,(int)fill,2,health ? 0xff64c84eu : 0xff409fe7u);
    Rect(pixels,2,y+5,(int)fill,2,health ? 0xff2b8737u : 0xff236cc1u);
}
static void Assist(uint32_t* pixels,unsigned gauge,unsigned family) {
    const char* name=sAssistNames[family<ASSIST_COUNT ? family : 0];
    unsigned fill=(gauge>ASSIST_MAX ? ASSIST_MAX : gauge)*GAUGE_WIDTH/ASSIST_MAX;
    int x=2,row,col;
    uint32_t ink=gauge>=ASSIST_MAX ? 0xffffe59au : 0xffd6c99cu;
    while(*name) {
        if(*name>='A' && *name<='Z')
            for(row=0;row<5;++row) for(col=0;col<3;++col)
                if(sLetter[*name-'A'][row]&(1u<<(2-col)))
                    Rect(pixels,x+col,20+row,1,1,ink);
        x+=4;++name;
    }
    Rect(pixels,1,27,56,6,0xff171a22u);
    Rect(pixels,2,27,54,1,0xffaaa183u);
    Rect(pixels,2,32,54,1,0xff6f6957u);
    Rect(pixels,2,28,54,4,0xff2c2930u);
    Rect(pixels,2,28,(int)fill,1,0xffffdf80u);
    Rect(pixels,2,29,(int)fill,2,gauge>=ASSIST_MAX ? 0xffefbd4du : 0xffc99943u);
    Rect(pixels,2,31,(int)fill,1,0xff87632du);
}

/* Rendering reads current actor HP/maxHP without touching combat values.
 * All health fits in the single green bar, including maximum HP above 280. */
void PortActionHudDraw(SDL_Renderer* renderer,const PortActionState* state,
                       float originX,float originY,float scale) {
    uint32_t pixels[HUD_WIDTH*HUD_HEIGHT];
    SDL_BlendMode savedBlend;
    Uint8 savedR,savedG,savedB,savedA;
    unsigned fill,magic;
    int x,y,hp,maxHp,canvasW,canvasH,inset;
    float dockX,dockY;
    if(!renderer || !state || scale<=0 || !PortActionHudHealth(&hp,&maxHp) || maxHp<=0 ||
       !PortActionHudCanvas(&canvasW,&canvasH,&inset)) return;
    dockX=originX+(canvasW-PORT_ACTION_HUD_WIDTH+4)*scale;
    dockY=originY+(canvasH-PORT_ACTION_HUD_HEIGHT-inset)*scale;
    if(hp<0) hp=0;
    if(hp>maxHp) hp=maxHp;
    memset(pixels,0,sizeof(pixels));
    fill=(unsigned)hp*GAUGE_WIDTH/(unsigned)maxHp;
    if(hp>0 && !fill) fill=1; /* A living player never looks completely empty. */
    Gauge(pixels,0,fill,1);
    fill=(state->mp>PORT_ACTION_MAX_MP ? PORT_ACTION_MAX_MP : state->mp)*GAUGE_WIDTH/PORT_ACTION_MAX_MP;
    Gauge(pixels,9,fill,0);
    Assist(pixels,state->assistGauge,state->assistFamily);
    magic=state->magicIndex%PORT_ACTION_MAGIC_COUNT;
    for(y=0;y<7;++y) for(x=0;x<7;++x)
        if(sMagicGlyphs[magic][y]&(1u<<(6-x)))
            Rect(pixels,50+x,20+y,1,1,0xff17202bu);
    for(y=0;y<7;++y) for(x=0;x<7;++x)
        if(sMagicGlyphs[magic][y]&(1u<<(6-x)))
            Rect(pixels,49+x,19+y,1,1,sMagicColors[magic]);

    SDL_GetRenderDrawColor(renderer,&savedR,&savedG,&savedB,&savedA);
    SDL_GetRenderDrawBlendMode(renderer,&savedBlend);
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    /* The same cartridge-pixel transform as the native portrait. */
    for(y=0;y<HUD_HEIGHT;++y) {
        for(x=0;x<HUD_WIDTH;) {
            uint32_t color=pixels[y*HUD_WIDTH+x];
            int left=x++;
            while(x<HUD_WIDTH && pixels[y*HUD_WIDTH+x]==color) ++x;
            if(color) {
                SDL_FRect rect={dockX+left*scale,dockY+y*scale,
                                (x-left)*scale,scale};
                SDL_SetRenderDrawColor(renderer,(Uint8)(color>>16),(Uint8)(color>>8),
                                       (Uint8)color,(Uint8)(color>>24));
                SDL_RenderFillRect(renderer,&rect);
            }
        }
    }
    SDL_SetRenderDrawColor(renderer,savedR,savedG,savedB,savedA);
    SDL_SetRenderDrawBlendMode(renderer,savedBlend);
}
