#ifndef PORT_ACTION_PALETTE_H
#define PORT_ACTION_PALETTE_H
#include <stdint.h>
typedef struct SDL_Renderer SDL_Renderer;
/* No player-facing move menu; isolated dispatcher audit compatibility only. */
int PortActionPaletteActive(void);
int PortActionPaletteDiagnosticActive(void);
void PortActionPaletteReset(void);
void PortActionPaletteInput(uint16_t keys);
int PortActionPaletteEvent(const void* event);
void PortActionPaletteDraw(SDL_Renderer* renderer,int width,int height);
unsigned PortActionPaletteSelectedIndex(void);
#endif
