"""Production controller polling/resolve/tree with simulated device reports.

This tests actual button/axis mapping and action logic, not physical hardware.
No keyboard events or direct action intents drive the controller scenarios.
"""
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development/native/out/controller-tests'
OUT.mkdir(parents=True, exist_ok=True)
base = runpy.run_path(str(ROOT / 'port/tools/test_keyboard_tap_latch.py'))
code = base['code'].split('int main(void) {')[0]
code = code.replace('#include "port/port_action.h"', '#include "port/port_action.h"\n#include "port/port_action_routes.h"\n#include "port/pad_layout.h"')
code = code.replace('SDL_JoystickID* SDL_GetGamepads(int* count) { *count=0;return NULL; }',
                    'SDL_JoystickID* SDL_GetGamepads(int* count) { *count=1;SDL_JoystickID* id=malloc(sizeof(*id));*id=1;return id; }')
code = code.replace('(void)id;return NULL;', '(void)id;return (SDL_Gamepad*)(uintptr_t)1;')
poll = r'''
static bool buttons[SDL_GAMEPAD_BUTTON_COUNT];
static Sint16 axes[SDL_GAMEPAD_AXIS_COUNT];
static SDL_GamepadType padType=SDL_GAMEPAD_TYPE_XBOXONE;
bool SDL_GetGamepadButton(SDL_Gamepad* pad,SDL_GamepadButton button) { (void)pad;return buttons[button]; }
Sint16 SDL_GetGamepadAxis(SDL_Gamepad* pad,SDL_GamepadAxis axis) { (void)pad;return axes[axis]; }
SDL_GamepadType SDL_GetGamepadType(SDL_Gamepad* pad) { (void)pad;return padType; }
'''
code = code.replace('static uint32_t GamepadKeys(SDL_Gamepad* pad) { (void)pad;return 0; }',
                    poll + 'static uint32_t GamepadKeys(SDL_Gamepad* pad);')
code += base['function']('static uint32_t GamepadKeys(')
code += base['function']('uint16_t PortInputActionHeld(')
code += base['function']('uint16_t PortInputActionPressed(')
config = (ROOT / 'port/src/port_config.c').read_text()
start = config.index('    {\n        SDL_GAMEPAD_BUTTON_SOUTH')
end = config.index('\n    },', start) + len('\n    }')
code += '\nstatic const int productionPad[PORT_ACT_COUNT]=' + config[start:end].strip() + ';\n'
code += r'''
static PortActionState action;
static uint16_t previous;
static unsigned accepted;
static PortActionIntent last;
static unsigned frame(int ready) {
    unsigned raw=PortInputKeys(),keys=PortInputResolve(raw);
    PortActionInput input={0};input.canAct=(uint8_t)ready;
    input.held=PortInputActionHeld();input.pressed=PortInputActionPressed();
    if(keys&1) input.held|=PORT_ACTION_ATTACK;
    if((keys&~previous)&1) input.pressed|=PORT_ACTION_ATTACK;
    if((keys&~previous)&512) input.pressed|=PORT_ACTION_PREV_MAGIC;
    if((keys&~previous)&256) input.pressed|=PORT_ACTION_NEXT_MAGIC;
    if(keys&16) input.held|=PORT_ACTION_RIGHT;
    if(keys&32) input.held|=PORT_ACTION_LEFT;
    if(keys&64) input.held|=PORT_ACTION_UP;
    if(keys&128) input.held|=PORT_ACTION_DOWN;
    previous=(uint16_t)keys;PortActionTick(&action,&input);
    if(PortActionPeek(&action,&last)) { assert(PortActionCommit(&action,&last));++accepted; }
    return keys;
}
static void fresh(void) {
    memset(buttons,0,sizeof(buttons));memset(axes,0,sizeof(axes));
    memset(&gPortConfig,0,sizeof(gPortConfig));memcpy(gPortConfig.pad,productionPad,sizeof(productionPad));
    gPortConfig.dodgeButton=gPortConfig.stockButton=1;gPortConfig.padLayout=1;
    actionActive=1;PortInputResetBindings();PortInputResolve(PortInputKeys());PortActionReset(&action);
    accepted=0;previous=0;
}
int main(void) {
    fresh();
    assert(productionPad[PORT_ACT_GUARD]==SDL_GAMEPAD_BUTTON_NORTH);
    assert(productionPad[PORT_ACT_TECHNIQUE]==110 && productionPad[PORT_ACT_MODIFIER]==108);
    for(unsigned step=1;step<=3;++step) {
        buttons[SDL_GAMEPAD_BUTTON_SOUTH]=true;frame(1);assert(accepted==step && last.comboStep==step);
        buttons[SDL_GAMEPAD_BUTTON_SOUTH]=false;frame(0);
    }
    buttons[SDL_GAMEPAD_BUTTON_SOUTH]=true;
    for(unsigned n=0;n<16;++n) frame(1);
    assert(accepted==3);buttons[SDL_GAMEPAD_BUTTON_SOUTH]=false;frame(1);
    assert(accepted==4 && last.command==PORT_ACTION_NATIVE_TECHNIQUE);
    assert(last.nativeMove==PortActionRouteFinisher(0,0,0,0,16,0));
    for(unsigned n=0;n<200;++n) frame(0);assert(accepted==4);

    fresh();buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER]=true;frame(1);
    assert(action.magicIndex==6);for(unsigned n=0;n<10;++n) frame(1);assert(action.magicIndex==6);
    buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER]=false;frame(1);
    buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER]=true;frame(1);assert(action.magicIndex==0 && !accepted);
    buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER]=false;frame(1);
    axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]=15999;frame(1);assert(!action.magicCharging);
    axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]=25000;
    for(unsigned n=0;n<16;++n) frame(1);
    assert(action.magicCharging && !accepted);axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]=0;frame(1);
    assert(accepted==1 && last.nativeMove==PortActionRouteMagic(0,0,0,16,0));

    fresh();axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER]=25000;axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]=25000;
    frame(1);axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]=0;frame(1);
    assert(accepted==1 && last.nativeMove==PortActionRouteMagic(0,0,0,2,1));
    fresh();buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT]=true;assert(!(frame(1)&(16|32|64|128)));
    assert(action.assistFamily==1);buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT]=false;frame(1);
    buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN]=true;assert(!(frame(1)&(16|32|64|128)));
    assert(action.keybladeIndex==1);buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN]=false;frame(1);
    axes[SDL_GAMEPAD_AXIS_LEFTX]=24000;assert(frame(1)&16);
    axes[SDL_GAMEPAD_AXIS_LEFTX]=-24000;assert(frame(1)&32);
    axes[SDL_GAMEPAD_AXIS_LEFTX]=0;PortActionAddAssistGauge(&action,100);
    buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK]=true;frame(1);
    buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK]=false;frame(1);
    uint32_t move;assert(PortActionPendingAssist(&action,&move));
    assert(move==PortActionRouteAssist(1,2,0) && action.assistGauge==100);
    assert(PortActionCommitAssist(&action,move) && action.assistGauge==0);
    fresh();buttons[SDL_GAMEPAD_BUTTON_NORTH]=true;frame(1);assert(PortActionGuardHeld());
    buttons[SDL_GAMEPAD_BUTTON_NORTH]=false;frame(1);assert(!PortActionGuardHeld());
    buttons[SDL_GAMEPAD_BUTTON_WEST]=true;frame(1);assert(PortTakeDodge(16,0)==16);
    fresh();buttons[SDL_GAMEPAD_BUTTON_WEST]=true;frame(0);
    buttons[SDL_GAMEPAD_BUTTON_WEST]=false;
    for(unsigned n=0;n<8;++n) {frame(0);assert(PortPeekDodge(16,0)==16);}
    assert(PortTakeDodge(16,0)==16 && !PortPeekDodge(16,0));
    fresh();buttons[SDL_GAMEPAD_BUTTON_WEST]=true;frame(0);
    buttons[SDL_GAMEPAD_BUTTON_WEST]=false;
    for(unsigned n=0;n<10;++n) frame(0);
    assert(!PortPeekDodge(16,0));
    puts("PASS: production controller buttons/axes -> resolve -> combo tree, trigger charge, modifier, D-pad selection, earned assist, guard and roll. Device reports simulated; physical pad not asserted.");
    return 0;
}
'''
(OUT / 'controller.c').write_text(code)
sdk = next((ROOT.parent / 'toolchain').glob('SDL3-*'))
vcvars = Path('C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvars64.bat')
command = ('cl /nologo /std:c11 /W3 /wd4309 /wd4341 /I"' + str(ROOT / 'port/include') +
           '" /I"' + str(ROOT / 'include/card') + '" /I"' + str(sdk / 'x86_64-w64-mingw32/include') +
           '" /Fe"' + str(OUT / 'controller.exe') + '" /Fo"' + OUT.as_posix() + '/" "' +
           str(OUT / 'controller.c') + '" "' + str(ROOT / 'port/src/port_action.c') + '" "' +
           str(ROOT / 'port/src/port_action_routes.c') + '" "' + str(ROOT / 'port/src/port_action_catalog.c') + '"')
batch = OUT / 'run.cmd'
batch.write_text('@echo off\ncall "' + str(vcvars) + '" >nul\nif errorlevel 1 exit /b 1\n' + command +
                 '\nif errorlevel 1 exit /b 1\n"' + str(OUT / 'controller.exe') + '"\n')
subprocess.run(['cmd.exe', '/c', str(batch)], cwd=ROOT, check=True, timeout=45)
