"""ROM-free regression for complete keyboard taps between game-frame samples.

The keyboard callback, sampling, resolve and reset functions are extracted from
the production source. Only unrelated device polling is stubbed: this test does
not stand in for a real-window keyboard or controller integration check.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/keyboard-tap-latch'
OUT.mkdir(parents=True, exist_ok=True)
source = (ROOT / 'port/src/port_input.c').read_text(encoding='utf-8')


def function(marker):
    start = source.index(marker)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


definitions = source[source.index('#define KEY_A'):source.index('/* GBA key bit')]
state = source[source.index('static const uint32_t sActionBits'):source.index('/* Activity is event-driven')]
code = r'''
#define SDL_STATIC_LIB 1
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include "port/port.h"
#include "port/port_action.h"
#ifdef NDEBUG
#error This regression requires active host assertions.
#endif
PortConfig gPortConfig;
static int actionActive=1, uiActive;
static int gModeBattle, gModeVsbattle;
static int* gCurrentMode;
int PortActionCombatActive(void) { return actionActive; }
int PortUiActive(void) { return uiActive; }
int PortActionPaletteActive(void) { return 0; }
int PortActionPaletteDiagnosticActive(void) { return 0; }
void PortActionPaletteInput(uint16_t keys) { (void)keys; }
void PortUiZoomStep(int direction) { (void)direction; }
int PortRollbackResimulating(void) { return 0; }
int VsBattleLocalFacingLeft(void) { return 0; }
SDL_JoystickID* SDL_GetGamepads(int* count) { *count=0;return NULL; }
SDL_Gamepad* SDL_GetGamepadFromID(SDL_JoystickID id) { (void)id;return NULL; }
SDL_Gamepad* SDL_OpenGamepad(SDL_JoystickID id) { (void)id;return NULL; }
void SDL_free(void* value) { free(value); }
static void VirtualPadUpdate(void) {}
static uint32_t GamepadKeys(SDL_Gamepad* pad) { (void)pad;return 0; }
'''
code += definitions + state
for marker in ('void PortInputSuppressUntilRelease(', 'void PortInputResetBindings(',
               'void PortInputHandleEvent(', 'uint32_t PortInputKeys(',
               'static uint32_t ZoomChord(', 'static void DeckButton(',
               'uint16_t PortInputResolve(', 'int PortActionGuardHeld('):
    code += function(marker)
code += r'''
static void event(SDL_Keycode key, int down, int repeat) {
    SDL_Event e={0};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
    e.key.key=key;e.key.repeat=repeat!=0;PortInputHandleEvent(&e);
}
static void reset(void) {
    PortInputResetBindings();assert(!PortInputKeys());PortInputResolve(0);
}
int main(void) {
    gPortConfig.keys[PORT_ACT_A][0]=SDLK_X;
    gPortConfig.keys[PORT_ACT_GUARD][0]=SDLK_V;
    gPortConfig.keys[PORT_ACT_UNSTOCK][0]=SDLK_V;
    gPortConfig.dodgeButton=gPortConfig.stockButton=1;
    reset();
    for(int i=0;i<3;i++) {
        event(SDLK_X,1,0);event(SDLK_X,0,0);
        assert(PortInputKeys()==KEY_A && PortInputKeys()==KEY_A);
        assert(PortInputResolve(PortInputKeys())==KEY_A);
        assert(!PortInputKeys());assert(!PortInputResolve(PortInputKeys()));
    }
    event(SDLK_X,1,0);
    assert(PortInputResolve(PortInputKeys())==KEY_A);
    unsigned previous=KEY_A,extraEdges=0;
    for(int i=0;i<60;i++) {
        event(SDLK_X,1,1);
        unsigned held=PortInputResolve(PortInputKeys());
        extraEdges+=(held&~previous&KEY_A)!=0;previous=held;
    }
    assert(!extraEdges);
    event(SDLK_X,0,0);assert(!PortInputResolve(PortInputKeys()));
    event(SDLK_X,1,0);event(SDLK_X,0,0);
    SDL_Event lost={0};lost.type=SDL_EVENT_WINDOW_FOCUS_LOST;
    PortInputHandleEvent(&lost);assert(!PortInputKeys());PortInputResolve(0);
    event(SDLK_X,1,0);event(SDLK_X,0,0);reset();assert(!PortInputKeys());
    event(SDLK_X,1,0);event(SDLK_X,0,0);PortInputSuppressUntilRelease();assert(!PortInputKeys());
    /* A new pulse after the overlay is not mistaken for an old held key. */
    event(SDLK_X,1,0);event(SDLK_X,0,0);
    assert(PortInputResolve(PortInputKeys())==KEY_A);assert(!PortInputResolve(PortInputKeys()));
    gPortConfig.keys[PORT_ACT_A][0]=SDLK_K;reset();
    event(SDLK_X,1,0);event(SDLK_X,0,0);assert(!PortInputResolve(PortInputKeys()));
    event(SDLK_K,1,0);event(SDLK_K,0,0);
    assert(PortInputResolve(PortInputKeys())==KEY_A);assert(!PortInputResolve(PortInputKeys()));
    /* A quick guard tap lasts one sample, while a real hold remains held. */
    reset();event(SDLK_V,1,0);event(SDLK_V,0,0);
    assert(!PortInputResolve(PortInputKeys()) && PortActionGuardHeld());
    assert(!PortInputResolve(PortInputKeys()) && !PortActionGuardHeld());
    event(SDLK_V,1,0);
    for(int i=0;i<30;i++)assert(!PortInputResolve(PortInputKeys()) && PortActionGuardHeld());
    event(SDLK_V,0,0);PortInputResolve(PortInputKeys());assert(!PortActionGuardHeld());
    actionActive=0;reset();event(SDLK_V,1,0);event(SDLK_V,0,0);
    assert(!PortInputResolve(PortInputKeys()) && !PortActionGuardHeld());
    puts("Production keyboard: three between-frame taps delivered once each; UI reads retain pulses; held/repeat has no extra edges; focus/reset/overlay wipe; remap; quick/held Guard PASS");
    return 0;
}
'''
(OUT / 'keyboard-tap-latch.c').write_text(code, encoding='utf-8')
sdk = next((p for p in (ROOT.parent / 'toolchain').glob('SDL3-*') if p.is_dir()), None)
if sdk is None:
    raise SystemExit('The official SDL3 development headers are required under ../toolchain/SDL3-*')
vcvars = Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat'
batch = OUT / 'run.cmd'
batch.write_text(
    '@echo off\ncall "' + str(vcvars) + '" >nul\nif errorlevel 1 exit /b 1\n' +
    'cl /nologo /std:c11 /W3 /I"' + str(ROOT / 'port/include') + '" /I"' +
    str(sdk / 'x86_64-w64-mingw32/include') + '" /Fe"' + str(OUT / 'keyboard-tap-latch.exe') +
    '" /Fo"' + str(OUT / 'keyboard-tap-latch.obj') + '" "' + str(OUT / 'keyboard-tap-latch.c') +
    '"\nif errorlevel 1 exit /b 1\n"' + str(OUT / 'keyboard-tap-latch.exe') + '"\n', encoding='utf-8')
subprocess.run(['cmd.exe', '/c', str(batch)], cwd=ROOT, check=True, timeout=45)
