/* Runs the production compact HUD renderer against an in-memory SDL target.
 * Compile with -Iport/include -idirafter port/ps5/guest/include, so the SDL
 * bridge declarations are available but the real host CRT/assert remain. */
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "port/port_action_hud.h"
#if defined(G_ASSERT_H) || defined(NDEBUG)
#error This renderer test requires the host CRT's active assertions.
#endif

enum { WIDTH=1280, HEIGHT=720 };
struct SDL_Renderer {
    uint32_t pixels[WIDTH*HEIGHT];
    Uint8 r,g,b,a;
    SDL_BlendMode blend;
};
static struct SDL_Renderer sRenderer;
static int sHp=560,sMaxHp=560,sHealthValid=1;
static int sCanvasW=240,sCanvasH=160,sInset,sCanvasValid=1;
int PortActionHudHealth(int* hp,int* maxHp) {
    *hp=sHp;*maxHp=sMaxHp;return sHealthValid;
}
int PortActionHudCanvas(int* width,int* height,int* inset) {
    *width=sCanvasW;*height=sCanvasH;*inset=sInset;return sCanvasValid;
}

bool SDL_SetRenderDrawColor(SDL_Renderer* r,Uint8 red,Uint8 green,Uint8 blue,Uint8 alpha) {
    r->r=red;r->g=green;r->b=blue;r->a=alpha;return true;
}
bool SDL_GetRenderDrawColor(SDL_Renderer* r,Uint8* red,Uint8* green,Uint8* blue,Uint8* alpha) {
    *red=r->r;*green=r->g;*blue=r->b;*alpha=r->a;return true;
}
bool SDL_SetRenderDrawBlendMode(SDL_Renderer* r,SDL_BlendMode mode) {r->blend=mode;return true;}
bool SDL_GetRenderDrawBlendMode(SDL_Renderer* r,SDL_BlendMode* mode) {*mode=r->blend;return true;}
bool SDL_RenderFillRect(SDL_Renderer* r,const SDL_FRect* rect) {
    int x,y;
    assert(rect->x>=0 && rect->y>=0 && rect->x+rect->w<=WIDTH && rect->y+rect->h<=HEIGHT);
    for(y=(int)rect->y;y<(int)(rect->y+rect->h);++y)
        for(x=(int)rect->x;x<(int)(rect->x+rect->w);++x)
            r->pixels[y*WIDTH+x]=((uint32_t)r->a<<24)|((uint32_t)r->r<<16)|((uint32_t)r->g<<8)|r->b;
    return true;
}

static void Draw(PortActionState* state,int ox,int oy,int scale) {
    PortActionState before=*state;
    unsigned x,y;
    memset(sRenderer.pixels,0,sizeof(sRenderer.pixels));
    sRenderer.r=7;sRenderer.g=17;sRenderer.b=27;sRenderer.a=137;
    sRenderer.blend=SDL_BLENDMODE_ADD;
    PortActionHudDraw(&sRenderer,state,(float)ox,(float)oy,(float)scale);
    assert(!memcmp(state,&before,sizeof(before)));
    assert(sRenderer.r==7 && sRenderer.g==17 && sRenderer.b==27 && sRenderer.a==137);
    assert(sRenderer.blend==SDL_BLENDMODE_ADD);
    for(y=0;y<HEIGHT;++y) for(x=0;x<WIDTH;++x) if(sRenderer.pixels[y*WIDTH+x]) {
        assert(x>=(unsigned)(ox+(sCanvasW-PORT_ACTION_HUD_WIDTH+4)*scale));
        assert(x<(unsigned)(ox+(sCanvasW-PORT_ACTION_HUD_WIDTH+74)*scale));
        assert(y>=(unsigned)(oy+(sCanvasH-PORT_ACTION_HUD_HEIGHT-sInset)*scale));
        assert(y<(unsigned)(oy+(sCanvasH-2-sInset)*scale));
    }
    /* The former upper-left cluster must stay completely empty. */
    for(y=0;y<40;++y) for(x=0;x<104;++x) assert(!sRenderer.pixels[y*WIDTH+x]);
}

static unsigned BluePixels(void) {
    unsigned i,count=0;
    for(i=0;i<WIDTH*HEIGHT;++i)
        count+=sRenderer.pixels[i]==0xffa5ddffu || sRenderer.pixels[i]==0xff409fe7u || sRenderer.pixels[i]==0xff236cc1u;
    return count;
}

static unsigned GreenPixels(void) {
    unsigned i,count=0;
    for(i=0;i<WIDTH*HEIGHT;++i)
        count+=sRenderer.pixels[i]==0xffb7f59au || sRenderer.pixels[i]==0xff64c84eu || sRenderer.pixels[i]==0xff2b8737u;
    return count;
}
static unsigned AmberPixels(void) {
    unsigned i,count=0;
    for(i=0;i<WIDTH*HEIGHT;++i)
        count+=sRenderer.pixels[i]==0xffffdf80u || sRenderer.pixels[i]==0xffc99943u ||
            sRenderer.pixels[i]==0xffefbd4du || sRenderer.pixels[i]==0xff87632du;
    return count;
}

static void Write16(FILE* file,unsigned value) {
    fputc(value&255,file);fputc((value>>8)&255,file);
}
static void Write32(FILE* file,unsigned value) {
    Write16(file,value&65535);Write16(file,value>>16);
}
/* Screenshots of the actual production renderer under controlled health.
 * These are render fixtures, not claims about inflicted gameplay damage. */
static void SaveFixture(const char* path) {
    enum { W=640,H=360 };
    FILE* file=fopen(path,"wb");int x,y;
    assert(file);fputc('B',file);fputc('M',file);Write32(file,54+W*H*4);
    Write32(file,0);Write32(file,54);Write32(file,40);Write32(file,W);Write32(file,H);
    Write16(file,1);Write16(file,32);Write32(file,0);Write32(file,W*H*4);
    Write32(file,0);Write32(file,0);Write32(file,0);Write32(file,0);
    for(y=H-1;y>=0;--y) for(x=0;x<W;++x)
        Write32(file,sRenderer.pixels[y*WIDTH+x] ? sRenderer.pixels[y*WIDTH+x] : 0xff303a4au);
    assert(!ferror(file));assert(!fclose(file));
}

int main(void) {
    PortActionState state;
    unsigned magic,x,y,hashes[PORT_ACTION_MAGIC_COUNT],assistHashes[14];
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
#endif
    memset(&state,0,sizeof(state));
    Draw(&state,0,0,1);assert(BluePixels()==0);assert(GreenPixels()==270);
    state.mp=PORT_ACTION_MAX_MP/2;Draw(&state,0,0,1);assert(BluePixels()==135);
    state.mp=PORT_ACTION_MAX_MP;Draw(&state,0,0,1);assert(BluePixels()==270);
    state.mp=PORT_ACTION_MAX_MP+1;Draw(&state,0,0,1);assert(BluePixels()==270);
    assert(AmberPixels()==0);
    state.assistGauge=50;Draw(&state,0,0,1);assert(AmberPixels()==108);
    state.assistGauge=100;Draw(&state,0,0,1);assert(AmberPixels()==216);
    state.assistGauge=65535;Draw(&state,0,0,1);assert(AmberPixels()==216);
    state.assistGauge=0;
    sHp=420;Draw(&state,0,0,1);assert(GreenPixels()==200); /* all HP, including extra tier */
    sHp=280;Draw(&state,0,0,1);assert(GreenPixels()==135);
    sHp=56;Draw(&state,0,0,1);assert(GreenPixels()==25);
    sHp=1;Draw(&state,0,0,1);assert(GreenPixels()==5);
    sHp=0;Draw(&state,0,0,1);assert(GreenPixels()==0);
    sHp=-10;Draw(&state,0,0,1);assert(GreenPixels()==0);
    sHp=700;Draw(&state,0,0,1);assert(GreenPixels()==270);
    sMaxHp=140;sHp=70;Draw(&state,0,0,1);assert(GreenPixels()==135);
    sMaxHp=560;sHp=560;
    for(magic=0;magic<PORT_ACTION_MAGIC_COUNT;++magic) {
        unsigned count=0,h=2166136261u,other;
        state.magicIndex=magic;Draw(&state,11,17,1);
        for(y=0;y<7;++y) for(x=0;x<7;++x) {
            uint32_t pixel=sRenderer.pixels[(17+sCanvasH-PORT_ACTION_HUD_HEIGHT+19+y)*WIDTH+
                11+sCanvasW-PORT_ACTION_HUD_WIDTH+53+x];
            unsigned ink=pixel && pixel!=0xff17202bu;
            count+=ink;h=(h^ink)*16777619u;
        }
        assert(count>=10 && count<40);hashes[magic]=h;
        for(other=0;other<magic;++other) assert(hashes[other]!=h);
    }
    for(magic=0;magic<14;++magic) {
        unsigned h=2166136261u,count=0,other;
        state.assistFamily=magic;Draw(&state,0,0,1);
        for(y=0;y<5;++y) for(x=0;x<44;++x) {
            uint32_t pixel=sRenderer.pixels[(sCanvasH-PORT_ACTION_HUD_HEIGHT+20+y)*WIDTH+
                sCanvasW-PORT_ACTION_HUD_WIDTH+6+x];
            count+=pixel!=0;h=(h^(pixel!=0))*16777619u;
        }
        assert(count>=10 && count<180);assistHashes[magic]=h;
        for(other=0;other<magic;++other) assert(h!=assistHashes[other]);
    }
    state.assistFamily=255;Draw(&state,0,0,1); /* invalid snapshots remain bounded */
    state.magicIndex=2;state.mp=50;Draw(&state,12,16,4);assert(BluePixels()==135*16);
    for(y=0;y<HEIGHT;y+=4) for(x=0;x<WIDTH;x+=4) {
        unsigned xx,yy;
        for(yy=0;yy<4;++yy) for(xx=0;xx<4;++xx)
            assert(sRenderer.pixels[(y+yy)*WIDTH+x+xx]==sRenderer.pixels[y*WIDTH+x]);
    }
    /* Wide canvas and raised separate-HUD layout use the same dock for every
     * gauge. Empty/partial/full assist never adds a second resource bar. */
    sCanvasW=320;sCanvasH=180;sInset=8;state.assistGauge=50;state.assistFamily=7;
    Draw(&state,0,0,4);assert(AmberPixels()==108*16);
    sInset=0;state.mp=100;state.magicIndex=0;sHp=560;state.assistGauge=100;
    Draw(&state,0,0,2);SaveFixture("build/hud-v10-full-health.bmp");
    sHp=280;state.mp=50;state.assistGauge=50;Draw(&state,0,0,2);SaveFixture("build/hud-v10-half-health.bmp");
    sHp=56;state.mp=15;state.assistGauge=0;Draw(&state,0,0,2);SaveFixture("build/hud-v10-low-health.bmp");
    memset(sRenderer.pixels,0,sizeof(sRenderer.pixels));
    PortActionHudDraw(NULL,&state,0,0,1);
    PortActionHudDraw(&sRenderer,NULL,0,0,1);
    PortActionHudDraw(&sRenderer,&state,0,0,0);
    sMaxHp=0;PortActionHudDraw(&sRenderer,&state,0,0,1);
    sMaxHp=560;sHealthValid=0;PortActionHudDraw(&sRenderer,&state,0,0,1);
    sHealthValid=1;sCanvasValid=0;PortActionHudDraw(&sRenderer,&state,0,0,1);
    for(x=0;x<WIDTH*HEIGHT;++x) assert(!sRenderer.pixels[x]);
    puts("PASS bottom-right HUD: one HP/MP pair,14 assist names/buildup,7 spell marks, empty old corner, pixel scaling and read-only rendering");
    return 0;
}
