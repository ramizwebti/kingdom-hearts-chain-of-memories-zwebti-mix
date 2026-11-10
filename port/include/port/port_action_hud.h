#ifndef GUARD_PORT_ACTION_HUD_H
#define GUARD_PORT_ACTION_HUD_H

#include <stdint.h>
#include "port/port_action.h"

typedef struct SDL_Renderer SDL_Renderer;

/* Read the active Sora actor's whole health range, not a cartridge HP tier.
 * Returns false outside a live action battle or before health is initialized. */
int PortActionHudHealth(int* hp, int* maxHp);
/* Actual canvas used by the native HUD this frame. The bottom inset includes
 * the existing raised-HUD preference only when a separate HUD layer uses it. */
int PortActionHudCanvas(int* width, int* height, int* bottomInset);

enum {
    PORT_ACTION_HUD_WIDTH=108,
    PORT_ACTION_HUD_HEIGHT=36,
    PORT_ACTION_PORTRAIT_RIGHT=32,
    PORT_ACTION_PORTRAIT_BOTTOM=36
};

/* Draw one total-HP bar, one MP bar and a small labeled assist buildup beside
 * the unchanged native Sora portrait, docked together at the bottom right.
 * Origin is the HUD canvas top-left in the current renderer viewport. The caller
 * owns battle/mode/scene gates. Drawing never advances gameplay state. */
void PortActionHudDraw(SDL_Renderer* renderer, const PortActionState* state,
                       float originX, float originY, float scale);

/* Legacy HD face helpers used by other platform overlays. */
int PortActionTextReady(SDL_Renderer* renderer, int code, float scale);
float PortActionTextAdvance(SDL_Renderer* renderer, int code, float scale);
void PortActionTextGlyph(SDL_Renderer* renderer, int code, float x, float y,
                         float scale, float cell, uint32_t face, uint32_t shadow);

#endif
