#include "port/port_action_fx.h"
#include <string.h>
#ifndef PORT_ACTION_FX_NO_RENDER
#include <SDL3/SDL.h>
#endif

/* Small fixed pool shared by contact and action cues. No allocations, game RNG
 * calls, sound ownership or task objects; safe even in an eight-enemy crowd. */
enum { FX_COUNT = 12, FX_IMPACT = PORT_ACTION_FX_CUE_COUNT };
typedef struct ActionImpact {
    int32_t x, y, z;
    uint8_t life, duration, power, seed, kind, left, variant;
} ActionImpact;
static ActionImpact sImpacts[FX_COUNT];
static unsigned sNext, sSerial;
enum { THREAT_COUNT = 3 };
typedef struct ActionThreat {
    uintptr_t key;
    int32_t x, y, groundZ, headZ;
    uint8_t remaining, total, ttl, left;
} ActionThreat;
static ActionThreat sThreats[THREAT_COUNT];

void PortActionFxReset(void) {
    memset(sImpacts, 0, sizeof(sImpacts));
    memset(sThreats, 0, sizeof(sThreats));
    sNext = sSerial = 0;
}
void PortActionFxTick(void) {
    unsigned i;
    for (i = 0; i < FX_COUNT; ++i)
        if (sImpacts[i].life) --sImpacts[i].life;
    for (i = 0; i < THREAT_COUNT; ++i)
        if (sThreats[i].ttl && !--sThreats[i].ttl) sThreats[i].key = 0;
}
void PortActionFxThreat(uintptr_t key, int32_t x, int32_t y, int32_t groundZ,
                        int32_t headZ, unsigned remaining, unsigned total, int facingLeft) {
    unsigned i, slot = THREAT_COUNT;
    ActionThreat* threat;
    if (!key) return;
    for (i = 0; i < THREAT_COUNT; ++i) {
        if (sThreats[i].key == key) { slot = i; break; }
        if (!sThreats[i].ttl) slot = i;
    }
    if (slot == THREAT_COUNT) return;
    threat = &sThreats[slot];
    if (!remaining || !total || remaining > total) {
        if (threat->key == key) memset(threat, 0, sizeof(*threat));
        return;
    }
    threat->key = key;
    threat->x = x; threat->y = y; threat->groundZ = groundZ; threat->headZ = headZ;
    threat->remaining = (uint8_t)(remaining > 255 ? 255 : remaining);
    threat->total = (uint8_t)(total > 255 ? 255 : total);
    threat->left = facingLeft != 0;
    /* Two ticks cover either player-before-enemy or enemy-before-player task
     * order, while disappearance/destruction has an explicit clear path. */
    threat->ttl = 2;
}
unsigned PortActionFxThreatCount(void) {
    unsigned i, count = 0;
    for (i = 0; i < THREAT_COUNT; ++i) count += sThreats[i].ttl != 0;
    return count;
}
static ActionImpact* FxAllocate(int32_t x, int32_t y, int32_t z) {
    unsigned i, index = sNext;
    ActionImpact* fx;
    /* Reuse an empty slot before replacing a visible effect. Under pressure,
     * evict the shortest remaining lifetime instead of a just-created burst. */
    for (i = 0; i < FX_COUNT; ++i) {
        unsigned candidate = (sNext + i) % FX_COUNT;
        if (!sImpacts[candidate].life) { index = candidate; break; }
        if (sImpacts[candidate].life < sImpacts[index].life) index = candidate;
    }
    fx = &sImpacts[index];
    memset(fx, 0, sizeof(*fx));
    fx->x = x; fx->y = y; fx->z = z;
    fx->seed = (uint8_t)(++sSerial);
    sNext = (index + 1) % FX_COUNT;
    return fx;
}
void PortActionFxHit(int32_t x, int32_t y, int32_t z, unsigned power) {
    static const uint8_t durations[3]={6,9,12};
    ActionImpact* hit;
    unsigned i;
    if(power>PORT_ACTION_FX_CONTACT_HEAVY) power=PORT_ACTION_FX_CONTACT_HEAVY;
    /* Several native hitboxes may resolve on one victim in the same tick.
     * Keep one truthful bright contact instead of additive white noise.
     * Already-aged contacts remain distinct beats of a multi-hit move. */
    for(i=0;i<FX_COUNT;++i) {
        hit=&sImpacts[i];
        if(hit->life && hit->life==hit->duration && hit->kind==FX_IMPACT &&
           hit->x==x && hit->y==y && hit->z==z) {
            if(power>hit->power) {
                hit->power=(uint8_t)power;
                hit->life=hit->duration=durations[power];
            }
            return;
        }
    }
    hit = FxAllocate(x, y, z);
    hit->kind = FX_IMPACT;
    hit->power = (uint8_t)power;
    hit->life = hit->duration = durations[power];
}
void PortActionFxCue(int32_t x, int32_t y, int32_t z,
                     unsigned cue, int facingLeft, unsigned variant) {
    static const uint8_t durations[PORT_ACTION_FX_CUE_COUNT] = {6, 8, 8, 6, 8};
    ActionImpact* fx;
    if (cue >= PORT_ACTION_FX_CUE_COUNT) return;
    fx = FxAllocate(x, y, z);
    fx->kind = (uint8_t)cue;
    fx->left = facingLeft != 0;
    fx->variant = (uint8_t)(cue == PORT_ACTION_FX_CAST ? variant % 7 : variant % 4);
    fx->duration = fx->life = durations[cue];
}
unsigned PortActionFxCount(void) {
    unsigned i, count = 0;
    for (i = 0; i < FX_COUNT; ++i) count += sImpacts[i].life != 0;
    return count;
}

#ifndef PORT_ACTION_FX_NO_RENDER
extern void WorldToScreen(int16_t* x, int16_t* y, int32_t wx, int32_t wy, int32_t wz);

/* Rendering reads simulation snapshots only. At most twelve short effects and
 * three small warning marks share this batch; no actor/task slots are used. */
enum { FX_VERTEX_MAX = 2048, FX_INDEX_MAX = FX_VERTEX_MAX / 4 * 6 };
typedef struct FxBatch {
    SDL_Vertex vertices[FX_VERTEX_MAX];
    int indices[FX_INDEX_MAX];
    unsigned used, indexCount;
} FxBatch;
static FxBatch sBatch;
/* Supplemental effects share the cartridge's pixel density. Rasterizing the
 * short ribbons here avoids smooth HD geometry over the original sprite art.
 * The texture is purely presentational: no game tasks, RNG or effect timers. */
enum { FX_PIXEL_WIDTH = 1024, FX_PIXEL_HEIGHT = 512 };
static uint32_t sPixels[FX_PIXEL_WIDTH * FX_PIXEL_HEIGHT];
static SDL_Texture* sPixelTexture;
static SDL_Renderer* sPixelRenderer;
static int sPixelWidth, sPixelHeight;

void PortActionFxRendererReset(void) {
    if (sPixelTexture) SDL_DestroyTexture(sPixelTexture);
    sPixelTexture = NULL;
    sPixelRenderer = NULL;
    sPixelWidth = sPixelHeight = 0;
}

static float FxCross(float ax,float ay,float bx,float by,float px,float py) {
    return (bx-ax)*(py-ay)-(by-ay)*(px-ax);
}
static int FxSample(const SDL_Vertex* a,const SDL_Vertex* b,const SDL_Vertex* c,
                    float x,float y,SDL_FColor* out) {
    float area=FxCross(a->position.x,a->position.y,b->position.x,b->position.y,
                       c->position.x,c->position.y);
    float u,v,w;
    if (area > -0.00001f && area < 0.00001f) return 0;
    u=FxCross(b->position.x,b->position.y,c->position.x,c->position.y,x,y)/area;
    v=FxCross(c->position.x,c->position.y,a->position.x,a->position.y,x,y)/area;
    w=1.0f-u-v;
    if (u<0 || v<0 || w<0) return 0;
    out->r=a->color.r*u+b->color.r*v+c->color.r*w;
    out->g=a->color.g*u+b->color.g*v+c->color.g*w;
    out->b=a->color.b*u+b->color.b*v+c->color.b*w;
    out->a=a->color.a*u+b->color.a*v+c->color.a*w;
    return 1;
}
static void FxRaster(int width,int height) {
    unsigned n;
    memset(sPixels,0,(size_t)width*height*sizeof(*sPixels));
    for(n=0;n<sBatch.used;n+=4) {
        const SDL_Vertex* v=&sBatch.vertices[n];
        int k,x,y,left=width,right=0,top=height,bottom=0;
        for(k=0;k<4;++k) {
            int xx=(int)v[k].position.x,yy=(int)v[k].position.y;
            if(xx<left) left=xx;
            if(xx+1>right) right=xx+1;
            if(yy<top) top=yy;
            if(yy+1>bottom) bottom=yy+1;
        }
        left=SDL_max(0,left);top=SDL_max(0,top);
        right=SDL_min(width,right);bottom=SDL_min(height,bottom);
        for(y=top;y<bottom;++y) for(x=left;x<right;++x) {
            SDL_FColor color;
            unsigned alpha,r,g,b;
            uint32_t old;
            /* Sample a quad once so its shared diagonal cannot overbrighten. */
            if(!FxSample(v,v+1,v+2,x+0.5f,y+0.5f,&color) &&
               !FxSample(v,v+2,v+3,x+0.5f,y+0.5f,&color)) continue;
            /* Eight brightness steps and RGB555 hue precision match the
             * limited, bright authored GBA effect palette. */
            alpha=(unsigned)(color.a*8.0f+0.5f); if(alpha>8) alpha=8;
            if(!alpha) continue;
            r=(unsigned)(color.r*31.0f+0.5f)*alpha;
            g=(unsigned)(color.g*31.0f+0.5f)*alpha;
            b=(unsigned)(color.b*31.0f+0.5f)*alpha;
            old=sPixels[y*width+x];
            r=SDL_min(255,((old>>16)&255)+r*255/(31*8));
            g=SDL_min(255,((old>>8)&255)+g*255/(31*8));
            b=SDL_min(255,(old&255)+b*255/(31*8));
            sPixels[y*width+x]=0xFF000000u|(r<<16)|(g<<8)|b;
        }
    }
}
static const float sCircle[17][2] = {
    {1,0},{0.92388f,0.38268f},{0.70711f,0.70711f},{0.38268f,0.92388f},
    {0,1},{-0.38268f,0.92388f},{-0.70711f,0.70711f},{-0.92388f,0.38268f},
    {-1,0},{-0.92388f,-0.38268f},{-0.70711f,-0.70711f},{-0.38268f,-0.92388f},
    {0,-1},{0.38268f,-0.92388f},{0.70711f,-0.70711f},{0.92388f,-0.38268f},{1,0}
};
static SDL_FColor FxColor(float r, float g, float b, float a) {
    SDL_FColor c;
    c.r=r; c.g=g; c.b=b; c.a=a;
    return c;
}
static void FxQuad(float x0,float y0,float x1,float y1,float x2,float y2,float x3,float y3,
                   SDL_FColor start, SDL_FColor end) {
    unsigned n = sBatch.used, k = sBatch.indexCount;
    SDL_Vertex* v;
    if (n + 4 > FX_VERTEX_MAX || k + 6 > FX_INDEX_MAX) return;
    v = &sBatch.vertices[n];
    memset(v, 0, 4 * sizeof(*v));
    v[0].position.x=x0; v[0].position.y=y0;
    v[1].position.x=x1; v[1].position.y=y1;
    v[2].position.x=x2; v[2].position.y=y2;
    v[3].position.x=x3; v[3].position.y=y3;
    v[0].color=v[1].color=start; v[2].color=v[3].color=end;
    sBatch.indices[k]=(int)n; sBatch.indices[k+1]=(int)n+1; sBatch.indices[k+2]=(int)n+2;
    sBatch.indices[k+3]=(int)n; sBatch.indices[k+4]=(int)n+2; sBatch.indices[k+5]=(int)n+3;
    sBatch.used+=4; sBatch.indexCount+=6;
}
static void FxRay(float x,float y,float dx,float dy,float length,float width,SDL_FColor c) {
    float nx=-dy*width, ny=dx*width;
    SDL_FColor tail = c;
    x=SDL_floorf(x)+0.5f; y=SDL_floorf(y)+0.5f;
    tail.a *= 0.06f;
    FxQuad(x+nx,y+ny,x-nx,y-ny,x+dx*length-nx*0.06f,y+dy*length-ny*0.06f,
           x+dx*length+nx*0.06f,y+dy*length+ny*0.06f,c,tail);
}
static void FxStar(float x,float y,float size,SDL_FColor c) {
    FxRay(x,y,1,0,size, size*0.15f,c); FxRay(x,y,-1,0,size,size*0.15f,c);
    FxRay(x,y,0,1,size*0.8f,size*0.12f,c); FxRay(x,y,0,-1,size*0.8f,size*0.12f,c);
}
static void FxArc(float x,float y,float rx,float ry,float width,unsigned start,unsigned count,
                  int mirror,SDL_FColor c) {
    unsigned i;
    float sign = mirror ? -1.0f : 1.0f;
    for (i=0; i<count; ++i) {
        unsigned a=(start+i)%16, b=(a+1)%16;
        float t = count == 16 ? 1.0f : (i+1)/(float)count;
        float w = width*t;
        SDL_FColor inner = c, outer = c;
        inner.a *= t; outer.a *= t*0.12f;
        FxQuad(x+sCircle[a][0]*(rx-w)*sign,y+sCircle[a][1]*(ry-w),
               x+sCircle[b][0]*(rx-w)*sign,y+sCircle[b][1]*(ry-w),
               x+sCircle[b][0]*(rx+w)*sign,y+sCircle[b][1]*(ry+w),
               x+sCircle[a][0]*(rx+w)*sign,y+sCircle[a][1]*(ry+w),inner,outer);
    }
}
static void FxStroke(float x0,float y0,float x1,float y1,float width,SDL_FColor c) {
    float dx,dy,length;
    float nx,ny;
    x0=SDL_floorf(x0)+0.5f; y0=SDL_floorf(y0)+0.5f;
    x1=SDL_floorf(x1)+0.5f; y1=SDL_floorf(y1)+0.5f;
    dx=x1-x0; dy=y1-y0; length=SDL_sqrtf(dx*dx+dy*dy);
    if(length<0.01f) return;
    nx=-dy*width/length; ny=dx*width/length;
    FxQuad(x0+nx,y0+ny,x0-nx,y0-ny,x1-nx,y1-ny,x1+nx,y1+ny,c,c);
}
static void FxBlade(float x,float y,float scale,float age,float fade,int left,unsigned variant) {
    /* Pixel-space paths follow the authored first, reverse, wide and overhead
     * swings. Only a short moving section survives, never an orbiting ring.
     * These describe blade motion; a miss cannot create a contact spark. */
    static const int8_t paths[4][7][2] = {
        {{-9,-13},{-2,-15},{6,-12},{12,-7},{15,0},{14,6},{10,10}},
        {{-7,12},{1,13},{9,9},{14,3},{15,-4},{10,-10},{3,-13}},
        {{-13,-7},{-7,-13},{3,-15},{13,-10},{18,-1},{16,7},{8,12}},
        {{0,-19},{7,-17},{12,-11},{14,-4},{13,4},{9,10},{4,13}}
    };
    const int8_t (*path)[2]=paths[variant%4];
    unsigned i,head=2+(unsigned)age,tail;
    float sign=left?-1.0f:1.0f;
    if(head>6) head=6;
    tail=head>4?head-4:0;
    for(i=tail;i<head;++i) {
        float strength=(i-tail+1)/(float)(head-tail);
        float x0=x+sign*path[i][0]*scale,y0=y+path[i][1]*scale;
        float x1=x+sign*path[i+1][0]*scale,y1=y+path[i+1][1]*scale;
        SDL_FColor edge=FxColor(0.35f,0.70f,1.0f,fade*strength*0.30f);
        SDL_FColor core=FxColor(1.0f,0.98f,0.78f,fade*strength*0.90f);
        FxStroke(x0,y0,x1,y1,0.95f*scale,edge);
        FxStroke(x0,y0,x1,y1,0.40f*scale,core);
    }
}
static SDL_FColor FxMagicColor(unsigned element,float alpha) {
    static const float colors[7][3] = {
        {1.0f,0.43f,0.12f}, {0.35f,0.85f,1.0f}, {1.0f,0.85f,0.22f},
        {0.35f,1.0f,0.53f}, {0.76f,0.40f,1.0f}, {0.95f,0.78f,0.38f}, {0.35f,1.0f,0.86f}
    };
    return FxColor(colors[element][0],colors[element][1],colors[element][2],alpha);
}
void PortActionFxDraw(SDL_Renderer* renderer,float originX,float originY,float scale) {
    int outW,outH,pixelStep,width,height;
    SDL_FRect dst;
    unsigned i,j;
    if (!renderer || scale<=0 || (!PortActionFxCount() && !PortActionFxThreatCount())) return;
    if (!SDL_GetCurrentRenderOutputSize(renderer,&outW,&outH) || outW<=0 || outH<=0) return;
    pixelStep=SDL_max(1,(int)scale);
    while((outW+pixelStep-1)/pixelStep>FX_PIXEL_WIDTH ||
          (outH+pixelStep-1)/pixelStep>FX_PIXEL_HEIGHT) ++pixelStep;
    width=(outW+pixelStep-1)/pixelStep;height=(outH+pixelStep-1)/pixelStep;
    if(sPixelTexture && (sPixelRenderer!=renderer || sPixelWidth!=width || sPixelHeight!=height))
        PortActionFxRendererReset();
    if(!sPixelTexture) {
        sPixelTexture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING,width,height);
        if(!sPixelTexture) return;
        sPixelRenderer=renderer;sPixelWidth=width;sPixelHeight=height;
        SDL_SetTextureScaleMode(sPixelTexture,SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(sPixelTexture,SDL_BLENDMODE_ADD);
    }
    originX/=pixelStep;originY/=pixelStep;scale/=pixelStep;
    sBatch.used=sBatch.indexCount=0;
    for (i=0; i<FX_COUNT; ++i) {
        const ActionImpact* fx = &sImpacts[i];
        int16_t sx,sy;
        float age,fade,x,y,sign;
        SDL_FColor gold,blue,white;
        if (!fx->life) continue;
        WorldToScreen(&sx,&sy,fx->x,fx->y,fx->z);
        age=(float)(fx->duration-fx->life); fade=fx->life/(float)fx->duration;
        x=originX+sx*scale; y=originY+sy*scale; sign=fx->left ? -1.0f : 1.0f;
        /* Keep accents at the cartridge's scale. Native magic/sleight effects
         * own their choreography; these marks only clarify motion or contact. */
        gold=FxColor(1.0f,0.74f,0.22f,fade*fade*0.80f);
        blue=FxColor(0.35f,0.68f,1.0f,fade*fade*0.45f);
        white=FxColor(1.0f,0.98f,0.80f,fade*fade);
        if (fx->kind==FX_IMPACT) {
            unsigned rays=4+fx->power;
            float offset=(0.8f+age*(0.30f+fx->power*0.15f))*scale;
            for (j=0;j<rays;++j) {
                unsigned d=(j*16/rays+(fx->seed&1))%16;
                float dx=sCircle[d][0],dy=sCircle[d][1];
                FxRay(x+dx*offset,y+dy*offset,dx,dy,
                      (1.0f+(2.0f+fx->power*2.0f)*fade)*scale,
                      (0.35f+fx->power*0.15f)*scale,j&1?white:gold);
            }
            /* Strong contact gets a short, weightier flash, not an expanding
             * full ring. No cue branch below emits this contact-only core. */
            if(age<3.0f)
                FxStar(x,y,((3.0f+fx->power*1.5f)-age)*scale,white);
        } else if (fx->kind==PORT_ACTION_FX_SLASH || fx->kind==PORT_ACTION_FX_FINISHER) {
            if(fx->kind==PORT_ACTION_FX_FINISHER && fx->variant==1) {
                FxRay(x-sign*3.0f*scale,y,-sign,0,14.0f*fade*scale,
                      0.45f*scale,white);
                FxRay(x-sign*5.0f*scale,y+3.0f*scale,-sign,0,
                      9.0f*fade*scale,0.40f*scale,blue);
            } else {
                FxBlade(x+sign*2.0f*scale,y,scale,age,fade,fx->left,
                    fx->kind==PORT_ACTION_FX_FINISHER?3:fx->variant);
            }
        } else if (fx->kind==PORT_ACTION_FX_CAST) {
            SDL_FColor magic=FxMagicColor(fx->variant,fade*0.70f);
            FxStar(x+sign*8.0f*scale,y-2.0f*scale,
                   (1.5f+fade)*scale,magic);
        } else if(fx->kind==PORT_ACTION_FX_DODGE) {
            for(j=0;j<2;++j) {
                float yy=y+((float)j*4.0f-2.0f)*scale;
                FxRay(x-sign*(age*0.4f+j*2.0f)*scale,yy,-sign,0,
                      (8.0f+j*3.0f)*fade*scale,0.40f*scale,blue);
            }
        } else if(fx->kind==PORT_ACTION_FX_GUARD) {
            FxArc(x+sign*3.0f*scale,y,5.0f*scale,7.0f*scale,
                  0.55f*scale,13,6,fx->left,blue);
            if(fx->variant && age<3.0f)
                FxStar(x+sign*8.0f*scale,y,(3.0f-age*0.5f)*scale,gold);
        }
    }
    /* Threats have a reserved pool and distinct warm color. They cannot be
     * evicted by a spell's hit sparks, and they end before its damage phase.
     * A short directional warning identifies the attacking actor without
     * placing a large ring underneath every crowded enemy. */
    for(i=0;i<THREAT_COUNT;++i) {
        const ActionThreat* threat=&sThreats[i];
        int16_t gx,gy,hx,hy;
        float x,y,headX,headY,ratio,reach;
        SDL_FColor amber;
        if(!threat->ttl) continue;
        WorldToScreen(&gx,&gy,threat->x,threat->y,threat->groundZ);
        WorldToScreen(&hx,&hy,threat->x,threat->y,threat->headZ);
        ratio=threat->remaining/(float)threat->total;
        x=originX+gx*scale; y=originY+gy*scale;
        headX=originX+hx*scale; headY=originY+hy*scale;
        reach=(6.0f+3.0f*ratio)*scale;
        amber=FxColor(1.0f,0.42f+0.28f*ratio,0.12f,0.75f);
        FxStroke(headX-scale,headY-3.0f*scale,headX,headY-scale,0.5f*scale,amber);
        FxStroke(headX+scale,headY-3.0f*scale,headX,headY-scale,0.5f*scale,amber);
        /* Only the committed lunge side gets a small ground chevron. */
        {
            float sign=threat->left?-1.0f:1.0f;
            FxStroke(x+sign*reach,y,x+sign*(reach-3.0f*scale),y-2.0f*scale,
                     0.5f*scale,amber);
            FxStroke(x+sign*reach,y,x+sign*(reach-3.0f*scale),y+2.0f*scale,
                     0.5f*scale,amber);
        }
    }
    if(sBatch.used) {
        FxRaster(width,height);
        if(!SDL_UpdateTexture(sPixelTexture,NULL,sPixels,width*sizeof(*sPixels))) return;
        dst.x=dst.y=0;dst.w=(float)(width*pixelStep);dst.h=(float)(height*pixelStep);
        SDL_RenderTexture(renderer,sPixelTexture,NULL,&dst);
    }
}
#endif
