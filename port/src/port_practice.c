#include "port/port_practice.h"
#include <string.h>
typedef struct PracticePreset {
    const char* key;
    const char* title;
    const char* detail;
    int entry;
} PracticePreset;
static const PracticePreset sPresets[PORT_PRACTICE_COUNT]={
    {"training","Training Target","A durable target for combos and magic.",206},
    {"crowd","Crowd Fight","Shadows, live pressure and finite waves.",0},
    {"mixed","Mixed Crowd","Soldiers and Shadows in a busy encounter.",4},
    {"axel","Axel Practice","Practice defense against a fast opponent.",186},
    {"jafar","Jafar Practice","Magic, positioning and boss reactions.",204}
};
static unsigned sSelection;
static int sActive,sMenu,sAutoStart;
int PortPracticeBegin(const char* name) {
    unsigned i;
    sActive=sMenu=sAutoStart=0;sSelection=0;
    if(!name) return 0;
    for(i=0;i<PORT_PRACTICE_COUNT;++i) if(!strcmp(name,sPresets[i].key)) {
        sSelection=i;sActive=sAutoStart=1;return 1;
    }
    return 0;
}
int PortPracticeActive(void) { return sActive; }
int PortPracticeEntry(void) { return sPresets[sSelection].entry; }
unsigned PortPracticeSelection(void) { return sSelection; }
const char* PortPracticeName(void) { return sPresets[sSelection].title; }
void PortPracticeEnterMenu(void) { if(sActive) sMenu=1; }
unsigned PortPracticeMenuInput(unsigned pressed) {
    if(!sActive || !sMenu) return 0;
    if(sAutoStart) { sAutoStart=0;return PORT_PRACTICE_START; }
    if(pressed&PORT_PRACTICE_BACK) return PORT_PRACTICE_BACK;
    if((pressed&(PORT_PRACTICE_UP|PORT_PRACTICE_DOWN))==PORT_PRACTICE_UP)
        sSelection=(sSelection+PORT_PRACTICE_COUNT-1)%PORT_PRACTICE_COUNT;
    else if((pressed&(PORT_PRACTICE_UP|PORT_PRACTICE_DOWN))==PORT_PRACTICE_DOWN)
        sSelection=(sSelection+1)%PORT_PRACTICE_COUNT;
    return pressed&PORT_PRACTICE_START;
}
void PortPracticeLaunch(void) { sMenu=0; }
void PortPracticeLeave(void) { sActive=sMenu=sAutoStart=0; }

#ifndef PORT_PRACTICE_NO_RENDER
#include <SDL3/SDL.h>
#include <stdio.h>
#include "port/port.h"
#include "port/port_action_game.h"
#include "port/port_pixel_ui.h"
#include "port/port_ui.h"
#include "sprite_palettes.h"

static void PracticeLabel(int action,char* text,size_t size,const char* fallback) {
    if(!PortControllerActionLabel(action,text,size)) SDL_strlcpy(text,fallback,size);
}

void PortPracticeDraw(SDL_Renderer* renderer,int width,int height) {
    static uint32_t pixels[1024*512];
    static const char* const details[PORT_PRACTICE_COUNT]={
        "Try combos and magic.","Fight waves of Shadows.",
        "Fight Soldiers and Shadows.","Practice close-range defense.",
        "Practice magic and reactions."
    };
    PortPixelCanvas canvas;
    char begin[24],back[24],up[24],down[24],text[128];
    int scale,x,y,i,cw,ch;
    if(!renderer || !sActive || width<240 || height<160) return;
    scale=PortPixelScale(width,height);
    /* Battle information belongs by the portrait. The arena remains available
     * through its existing input, without a permanent control-label overlay. */
    if(!sMenu) return;
    /* Render the original Castle Oblivion load-menu tiles across the complete
     * window, then place cartridge-sized menu windows on that native canvas. */
    cw=(width+scale-1)/scale;ch=(height+scale-1)/scale;
    if(cw>1024 || ch>512) return;
    canvas.pixels=pixels;canvas.width=cw;canvas.height=ch;
    PortPixelBackdrop(&canvas);
    x=(cw-224)/2;y=(ch-160)/2;
    PortPixelFrame(&canvas,x,y+4,224,32);
    PortPixelText(&canvas,x+8,y+5,136,"Kingdom Hearts",PORT_PIXEL_WHITE);
    PortPixelText(&canvas,x+8,y+19,136,"Chain of Memories",PORT_PIXEL_WHITE);
    PortPixelText(&canvas,x+146,y+5,70,"Zwebti Mix",PORT_PIXEL_CYAN);
    PortPixelText(&canvas,x+146,y+19,70,"Practice",PORT_PIXEL_WHITE);
    PortPixelFrame(&canvas,x,y+38,224,86);
    for(i=0;i<PORT_PRACTICE_COUNT;++i) {
        int yy=y+42+i*16,selected=(unsigned)i==sSelection;
        if(selected) {
            PortPixelRect(&canvas,x+6,yy-1,212,16,PortPixelColor(gMapMenuPanelPalette[4]));
            PortPixelText(&canvas,x+8,yy,10,"\342\206\222",PORT_PIXEL_YELLOW);
        }
        PortPixelText(&canvas,x+22,yy,194,sPresets[i].title,
                       selected ? PORT_PIXEL_YELLOW : PORT_PIXEL_WHITE);
    }
    PortPixelFrame(&canvas,x,y+126,224,30);
    PortPixelText(&canvas,x+8,y+130,208,details[sSelection],PORT_PIXEL_WHITE);
    PracticeLabel(PORT_ACT_A,begin,sizeof(begin),"X");
    PracticeLabel(PORT_ACT_B,back,sizeof(back),"Z");
    PracticeLabel(PORT_ACT_UP,up,sizeof(up),"Up");
    PracticeLabel(PORT_ACT_DOWN,down,sizeof(down),"Down");
    snprintf(text,sizeof(text),"%s/%s  %s Begin  %s Title",up,down,begin,back);
    PortPixelText(&canvas,x+8,y+143,208,text,PORT_PIXEL_CYAN);
    PortPixelPresent(renderer,PORT_PIXEL_PRACTICE,&canvas,0,0,scale);
}
#endif
