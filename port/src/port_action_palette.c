/* No combat move menu. Isolated native-dispatch audit hook only. */
#include <SDL3/SDL.h>
#include "port/port.h"
#include "port/port_action_palette.h"
#include "port/port_action_catalog.h"
#include "port/port_action_game.h"
static int sDiagnostic;
static unsigned sIndex;
static uint16_t sPrevious;
int PortActionPaletteDiagnosticActive(void) { return sDiagnostic; }
int PortActionPaletteActive(void) { return 0; }
unsigned PortActionPaletteSelectedIndex(void) { return sIndex; }
void PortActionPaletteReset(void) {
    const char* value=SDL_getenv("KHCOM_CATALOG_INDEX");
    sDiagnostic=0;sPrevious=0;
    if(value && SDL_getenv("KHCOM_PRACTICE")) {
        int index=SDL_atoi(value);
        if(index>=0 && (unsigned)index<PortActionCatalogCount()) {
            sIndex=(unsigned)index;sDiagnostic=1;
            PortActionSelectNativeMove(PortActionCatalogGet(sIndex)->nativeMove);
        }
    }
}
void PortActionPaletteInput(uint16_t keys) {
    uint16_t pressed=keys&~sPrevious;sPrevious=keys;
    if(sDiagnostic && PortActionCombatActive() && (pressed&PORT_KEY_TECHNIQUE))
        PortActionRequestNativeMove(PortActionCatalogGet(sIndex)->nativeMove);
}
int PortActionPaletteEvent(const void* event) { (void)event;return 0; }
void PortActionPaletteDraw(SDL_Renderer* renderer,int width,int height) {
    (void)renderer;(void)width;(void)height;
}
