#ifndef PORT_PRACTICE_H
#define PORT_PRACTICE_H
#include <stdint.h>
typedef struct SDL_Renderer SDL_Renderer;
enum { PORT_PRACTICE_COUNT=5 };
enum PortPracticeInput { PORT_PRACTICE_UP=1, PORT_PRACTICE_DOWN=2,
    PORT_PRACTICE_START=4, PORT_PRACTICE_BACK=8 };
/* Optional arena session; invalid/missing names leave normal boot. */
int PortPracticeBegin(const char* name);
int PortPracticeActive(void);
int PortPracticeEntry(void);
unsigned PortPracticeSelection(void);
const char* PortPracticeName(void);
void PortPracticeEnterMenu(void);
/* START/BACK requests a transition, including the initial automatic start. */
unsigned PortPracticeMenuInput(unsigned pressed);
void PortPracticeLaunch(void);
void PortPracticeLeave(void);
void PortPracticeDraw(SDL_Renderer* renderer,int width,int height);
#endif
