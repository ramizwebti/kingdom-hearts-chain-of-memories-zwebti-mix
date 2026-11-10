#ifndef PORT_PIXEL_UI_H
#define PORT_PIXEL_UI_H

#include <stdint.h>
typedef struct SDL_Renderer SDL_Renderer;

/* Coordinates are cartridge pixels. Only the final integer-scaled blit knows
 * the output resolution; artwork and glyphs never pass through HD text. */
typedef struct PortPixelCanvas { uint32_t* pixels; int width, height; } PortPixelCanvas;
enum PortPixelInk { PORT_PIXEL_WHITE, PORT_PIXEL_CYAN, PORT_PIXEL_YELLOW, PORT_PIXEL_GRAY };
enum PortPixelLayer { PORT_PIXEL_ACTION, PORT_PIXEL_PRACTICE, PORT_PIXEL_PRACTICE_HINT,
                      PORT_PIXEL_LAYER_COUNT };
uint32_t PortPixelColor(unsigned rgb555);
int PortPixelScale(int width, int height);
void PortPixelRect(PortPixelCanvas* canvas, int x, int y, int width, int height, uint32_t color);
void PortPixelText(PortPixelCanvas* canvas, int x, int y, int width,
                   const char* text, enum PortPixelInk ink);
void PortPixelFrame(PortPixelCanvas* canvas, int x, int y, int width, int height);
void PortPixelBackdrop(PortPixelCanvas* canvas);
void PortPixelPresent(SDL_Renderer* renderer, enum PortPixelLayer layer,
                      const PortPixelCanvas* canvas, int x, int y, int scale);
/* Called while servicing a renderer device reset, before the next draw. */
void PortPixelUiReset(void);

#endif
