/* Production has no finisher menu; only an explicit isolated audit hook. */
#define SDL_STATIC_LIB 1
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "port/port.h"
#include "port/port_action_catalog.h"
#include "port/port_action_palette.h"
static int active=1,practice,requests,selections;
static const char* indexValue;
static uint32_t selected,requested;
int PortActionCombatActive(void) { return active; }
int PortActionSelectNativeMove(uint32_t move) { selected=move;++selections;return 1; }
int PortActionRequestNativeMove(uint32_t move) { requested=move;++requests;return 1; }
const char* SDL_getenv(const char* key) {
    if(!strcmp(key,"KHCOM_CATALOG_INDEX")) return indexValue;
    if(!strcmp(key,"KHCOM_PRACTICE")) return practice ? "training" : NULL;
    return NULL;
}
int SDL_atoi(const char* text) { return atoi(text); }
int main(void) {
    PortActionPaletteReset();assert(!PortActionPaletteActive() && !PortActionPaletteDiagnosticActive());
    PortActionPaletteInput(PORT_KEY_MOVE_MENU);PortActionPaletteInput(0);
    PortActionPaletteInput(PORT_KEY_TECHNIQUE);assert(!requests && !selections);
    SDL_Event event={0};event.type=SDL_EVENT_KEY_DOWN;event.key.key=SDLK_ESCAPE;
    assert(!PortActionPaletteEvent(&event));event.key.key='1';assert(!PortActionPaletteEvent(&event));
    PortActionPaletteDraw(NULL,640,360);assert(!PortActionPaletteActive());
    indexValue="2";PortActionPaletteReset();assert(!PortActionPaletteDiagnosticActive());
    practice=1;PortActionPaletteReset();assert(PortActionPaletteDiagnosticActive());
    assert(selected==PortActionCatalogGet(2)->nativeMove);
    PortActionPaletteInput(PORT_KEY_TECHNIQUE);PortActionPaletteInput(PORT_KEY_TECHNIQUE);
    assert(requests==1 && requested==selected);
    PortActionPaletteInput(0);PortActionPaletteInput(PORT_KEY_TECHNIQUE);assert(requests==2);
    indexValue="99";PortActionPaletteReset();assert(selected==0);
    indexValue="18";PortActionPaletteReset();assert(selected>0x7fffffff);
    indexValue="124";PortActionPaletteReset();assert(!PortActionPaletteDiagnosticActive());
    indexValue=NULL;PortActionPaletteReset();assert(!PortActionPaletteDiagnosticActive() && !PortActionPaletteActive());
    puts("PASS: no player move menu/favorite hotkeys; explicit isolated catalog audit retains full native IDs.");return 0;
}
