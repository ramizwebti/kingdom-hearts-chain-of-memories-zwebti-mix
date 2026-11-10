#ifndef PORT_ACTION_FX_H
#define PORT_ACTION_FX_H
#include <stdint.h>
typedef struct SDL_Renderer SDL_Renderer;
/* Simulation-owned impact effects. Rendering never ages these particles or
 * advances the game's random generator. Capacity and lifetimes are bounded. */
void PortActionFxReset(void);
/* GPU cache invalidation; does not consume or clear simulation effects. */
void PortActionFxRendererReset(void);
void PortActionFxTick(void);
void PortActionFxHit(int32_t x, int32_t y, int32_t z, unsigned power);
enum PortActionFxCueKind {
    PORT_ACTION_FX_SLASH,
    PORT_ACTION_FX_FINISHER,
    PORT_ACTION_FX_CAST,
    PORT_ACTION_FX_DODGE,
    PORT_ACTION_FX_GUARD,
    PORT_ACTION_FX_CUE_COUNT
};
/* Emit only after an action is accepted, once per action. CAST variants match
 * the seven magic slots (Fire..Aero); GUARD variant 1 denotes a perfect guard.
 * Coordinates use the same world units as PortActionFxHit. */
void PortActionFxCue(int32_t x, int32_t y, int32_t z,
                     unsigned cue, int facingLeft, unsigned variant);
/* A windup snapshot is refreshed by a live enemy callback, not its AI range
 * query. Zero remaining removes it immediately. Keys are compared only;
 * rendering never dereferences an actor and stale snapshots expire. */
void PortActionFxThreat(uintptr_t key, int32_t x, int32_t y, int32_t groundZ,
                        int32_t headZ, unsigned remaining, unsigned total, int facingLeft);
unsigned PortActionFxThreatCount(void);
unsigned PortActionFxCount(void);
void PortActionFxDraw(SDL_Renderer* renderer, float originX, float originY, float scale);
#endif
