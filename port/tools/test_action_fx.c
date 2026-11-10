#include "port/port_action_fx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef PORT_ACTION_FX_RENDER_TEST
#include <SDL3/SDL.h>
void WorldToScreen(int16_t* x,int16_t* y,int32_t wx,int32_t wy,int32_t wz) {
    *x=(int16_t)wx; *y=(int16_t)(wy+wz);
}
typedef struct InkBounds {
    unsigned count;
    int left,top,right,bottom;
    uint32_t hash;
} InkBounds;
static void Clear(SDL_Renderer* renderer) {
    assert(SDL_SetRenderDrawColor(renderer,12,20,36,255));
    assert(SDL_RenderClear(renderer));
}
static InkBounds ReadInk(SDL_Surface* surface) {
    InkBounds ink={0,surface->w,surface->h,0,0,2166136261u};
    int x,y;
    for(y=0;y<surface->h;++y) for(x=0;x<surface->w;++x) {
        Uint8 r,g,b,a;
        assert(SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a));
        /* Additive marks must never darken the original battlefield. */
        assert(r>=12 && g>=20 && b>=36);
        ink.hash=(ink.hash^r)*16777619u;
        ink.hash=(ink.hash^g)*16777619u;
        ink.hash=(ink.hash^b)*16777619u;
        if(r==12 && g==20 && b==36) continue;
        ++ink.count;
        if(x<ink.left) ink.left=x;
        if(y<ink.top) ink.top=y;
        if(x>ink.right) ink.right=x;
        if(y>ink.bottom) ink.bottom=y;
    }
    return ink;
}
static void CheckRenderer(void) {
    SDL_Surface* surface=SDL_CreateSurface(320,160,SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer* renderer;
    SDL_BlendMode blend;
    SDL_Rect clip={20,12,270,132},after;
    Uint8 r,g,b,a;
    unsigned i,j,before;
    assert(surface);renderer=SDL_CreateSoftwareRenderer(surface);assert(renderer);
    Clear(renderer);
    assert(SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND));
    assert(SDL_SetRenderDrawColor(renderer,77,88,99,123));
    assert(SDL_SetRenderClipRect(renderer,&clip));
    PortActionFxReset();
    for(i=0;i<PORT_ACTION_FX_CUE_COUNT;++i)
        PortActionFxCue(40+(int)i*45,50,0,i,0,i);
    PortActionFxHit(260,50,0,2);
    PortActionFxThreat(1,100,110,0,-20,10,14,0);
    before=PortActionFxCount();
    PortActionFxDraw(NULL,0,0,1);
    PortActionFxDraw(renderer,0,0,0);
    PortActionFxDraw(renderer,0,0,-1);
    for(i=0;i<5;++i) PortActionFxDraw(renderer,0,0,1);
    assert(PortActionFxCount()==before && PortActionFxThreatCount()==1);
    assert(SDL_GetRenderDrawBlendMode(renderer,&blend) && blend==SDL_BLENDMODE_BLEND);
    assert(SDL_GetRenderDrawColor(renderer,&r,&g,&b,&a));
    assert(r==77 && g==88 && b==99 && a==123);
    assert(SDL_GetRenderClipRect(renderer,&after));
    assert(!memcmp(&clip,&after,sizeof(clip)));
    assert(SDL_FlushRenderer(renderer));assert(ReadInk(surface).count>0);
    /* Full-pool pressure, mirroring and edge clipping must remain bounded. */
    for(j=0;j<PORT_ACTION_FX_CUE_COUNT;++j) {
        PortActionFxReset();
        for(i=0;i<12;++i) PortActionFxCue(120,80,0,j,1,i);
        SDL_ClearError();PortActionFxDraw(renderer,0,0,1);
        assert(SDL_GetError()[0]=='\0' && PortActionFxCount()==12);
    }
    assert(SDL_SetRenderClipRect(renderer,NULL));Clear(renderer);
    clip.x=30;clip.y=20;clip.w=90;clip.h=60;
    assert(SDL_SetRenderClipRect(renderer,&clip));
    PortActionFxReset();PortActionFxHit(32,22,0,2);
    PortActionFxDraw(renderer,0,0,1);assert(SDL_FlushRenderer(renderer));
    for(j=0;j<(unsigned)surface->h;++j) for(i=0;i<(unsigned)surface->w;++i) {
        if(i>=30 && i<120 && j>=20 && j<80) continue;
        assert(SDL_ReadSurfacePixel(surface,i,j,&r,&g,&b,&a));
        assert(r==12 && g==20 && b==36);
    }
    PortActionFxRendererReset();SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);
    PortActionFxReset();
}
static void CheckContactOwnership(void) {
    SDL_Surface* surface=SDL_CreateSurface(160,120,SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer* renderer;
    InkBounds normal,final;
    unsigned i,x,y;
    assert(surface);renderer=SDL_CreateSoftwareRenderer(surface);assert(renderer);
    for(i=0;i<4;++i) {
        Clear(renderer);PortActionFxReset();
        PortActionFxCue(80,60,0,PORT_ACTION_FX_SLASH,0,i);
        PortActionFxTick();PortActionFxTick();PortActionFxDraw(renderer,0,0,1);
        assert(SDL_FlushRenderer(renderer));assert(ReadInk(surface).count>0);
        /* Misses draw only a blade ribbon, with no fabricated contact core. */
        for(y=58;y<=62;++y) for(x=78;x<=82;++x) {
            Uint8 r,g,b,a;
            assert(SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a));
            assert(r==12 && g==20 && b==36);
        }
    }
    Clear(renderer);PortActionFxReset();PortActionFxHit(80,60,0,0);
    PortActionFxDraw(renderer,0,0,1);assert(SDL_FlushRenderer(renderer));normal=ReadInk(surface);
    Clear(renderer);PortActionFxReset();PortActionFxHit(80,60,0,2);
    PortActionFxDraw(renderer,0,0,1);assert(SDL_FlushRenderer(renderer));final=ReadInk(surface);
    assert(normal.count>0 && final.count>normal.count);
    assert(final.right-final.left<28 && final.bottom-final.top<28);
    PortActionFxRendererReset();SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);
    PortActionFxReset();
}
static void CheckPixelGridAndVariants(void) {
    SDL_Surface* surface=SDL_CreateSurface(1280,720,SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer* renderer;
    uint32_t hashes[7];
    unsigned kind,i,j,x,y,xx,yy,before;
    assert(surface);renderer=SDL_CreateSoftwareRenderer(surface);assert(renderer);
    for(kind=0;kind<2;++kind) {
        unsigned count=kind?7:4;
        for(i=0;i<count;++i) {
            Clear(renderer);PortActionFxReset();
            PortActionFxCue(80,60,0,kind?PORT_ACTION_FX_CAST:PORT_ACTION_FX_SLASH,0,i);
            PortActionFxTick();PortActionFxTick();PortActionFxDraw(renderer,0,0,4);
            assert(SDL_FlushRenderer(renderer));
            { InkBounds ink=ReadInk(surface);assert(ink.count>0);hashes[i]=ink.hash; }
            for(j=0;j<i;++j) assert(hashes[i]!=hashes[j]);
        }
    }
    Clear(renderer);PortActionFxReset();
    for(i=0;i<4;++i) PortActionFxCue(35+(int)i*65,40,0,PORT_ACTION_FX_SLASH,i&1,i);
    PortActionFxTick();PortActionFxTick();PortActionFxDraw(renderer,0,0,4);
    assert(SDL_FlushRenderer(renderer));PortActionFxReset();
    PortActionFxHit(35,95,0,0);PortActionFxHit(100,95,0,2);
    PortActionFxCue(165,95,0,PORT_ACTION_FX_GUARD,0,1);
    PortActionFxCue(230,95,0,PORT_ACTION_FX_DODGE,0,0);
    PortActionFxTick();PortActionFxTick();PortActionFxDraw(renderer,0,0,4);
    assert(SDL_FlushRenderer(renderer));PortActionFxReset();
    /* Each row stays within the production pool, including all seven colors. */
    for(i=0;i<7;++i) PortActionFxCue(25+(int)i*43,145,0,PORT_ACTION_FX_CAST,0,i);
    PortActionFxTick();PortActionFxTick();before=PortActionFxCount();
    PortActionFxDraw(renderer,0,0,4);assert(SDL_FlushRenderer(renderer));
    for(y=0;y<720;y+=4) for(x=0;x<1280;x+=4) {
        Uint8 red,green,blue,alpha,r,g,b,a;
        assert(SDL_ReadSurfacePixel(surface,x,y,&red,&green,&blue,&alpha));
        for(yy=0;yy<4;++yy) for(xx=0;xx<4;++xx) {
            assert(SDL_ReadSurfacePixel(surface,x+xx,y+yy,&r,&g,&b,&a));
            assert(red==r && green==g && blue==b && alpha==a);
        }
    }
    assert(SDL_SavePNG(surface,"build/refined-fx-atlas.png"));
    PortActionFxRendererReset();assert(PortActionFxCount()==before);
    PortActionFxDraw(renderer,0,0,4);assert(PortActionFxCount()==before);
    PortActionFxRendererReset();SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);
    PortActionFxReset();
}
#endif
int main(void) {
    unsigned i;
    PortActionFxReset();assert(PortActionFxCount()==0);
    for(i=0;i<100;++i) PortActionFxHit((int)i,0,0,i);
    assert(PortActionFxCount()==12);
    for(i=0;i<11;++i) PortActionFxTick();assert(PortActionFxCount()==12);
    PortActionFxTick();assert(PortActionFxCount()==0);
    PortActionFxHit(0,0,0,0);PortActionFxHit(0,0,0,1);PortActionFxHit(0,0,0,2);
    for(i=0;i<10;++i) PortActionFxTick();assert(PortActionFxCount()==1);
    PortActionFxTick();PortActionFxTick();assert(PortActionFxCount()==0);
    for(i=0;i<PORT_ACTION_FX_CUE_COUNT;++i) PortActionFxCue(1,2,3,i,1,999);
    assert(PortActionFxCount()==5);
    PortActionFxCue(1,2,3,PORT_ACTION_FX_CUE_COUNT,0,0);
    PortActionFxCue(1,2,3,~0u,0,0);assert(PortActionFxCount()==5);
    for(i=0;i<6;++i) PortActionFxTick();assert(PortActionFxCount()==3);
    PortActionFxTick();PortActionFxTick();assert(PortActionFxCount()==0);
    /* An arriving finisher survives pressure from nearly expired sparks. */
    for(i=0;i<12;++i) PortActionFxHit(0,0,0,0);
    for(i=0;i<9;++i) PortActionFxTick();
    PortActionFxHit(0,0,0,2);PortActionFxTick();assert(PortActionFxCount()==1);
    for(i=0;i<11;++i) PortActionFxTick();assert(PortActionFxCount()==0);
    for(i=0;i<10000;++i) {
        PortActionFxCue((int)i,0,0,i%PORT_ACTION_FX_CUE_COUNT,i&1,i);
        PortActionFxHit(0,0,0,i);if(i%3==0) PortActionFxTick();
        assert(PortActionFxCount()<=12);
    }
    for(i=0;i<12;++i) PortActionFxTick();assert(PortActionFxCount()==0);
    /* Threat snapshots expire independently and never dereference actor keys. */
    PortActionFxThreat(0,0,0,0,0,10,14,0);assert(PortActionFxThreatCount()==0);
    for(i=1;i<=4;++i) PortActionFxThreat(i,0,0,0,-20,10,14,0);
    assert(PortActionFxThreatCount()==3);
    for(i=0;i<40;++i) PortActionFxHit(0,0,0,2);
    assert(PortActionFxThreatCount()==3);
    PortActionFxThreat(2,0,0,0,0,0,0,0);assert(PortActionFxThreatCount()==2);
    PortActionFxThreat(4,0,0,0,-20,10,14,1);assert(PortActionFxThreatCount()==3);
    PortActionFxTick();PortActionFxThreat(1,10,5,0,-30,4,14,0);PortActionFxTick();
    assert(PortActionFxThreatCount()==1);
    PortActionFxThreat(1,0,0,0,0,15,14,0);assert(PortActionFxThreatCount()==0);
    PortActionFxThreat(1,0,0,0,0,1,14,0);PortActionFxTick();PortActionFxTick();
    assert(PortActionFxThreatCount()==0);
    PortActionFxThreat(1,0,0,0,0,1,14,0);PortActionFxReset();
    assert(!PortActionFxCount() && !PortActionFxThreatCount());
#ifdef PORT_ACTION_FX_RENDER_TEST
    CheckRenderer();CheckContactOwnership();CheckPixelGridAndVariants();
#endif
    puts("PASS: bounded contact FX, finite cues, native pixels, clip/state preservation and threat expiry");
    return 0;
}
