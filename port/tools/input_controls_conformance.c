/* Production input path with a real SDL virtual gamepad; no cartridge needed. */
#include <assert.h>
#include <stdio.h>
#include "../src/port_input.c"
PortConfig gPortConfig = {
    .dodgeButton=1,.stockButton=1,.unstockButton=1,
    .keys={{SDLK_X,SDLK_K},{SDLK_Z,SDLK_J},{SDLK_A,0},{SDLK_S,0},
           {SDLK_RETURN,0},{SDLK_BACKSPACE,0},{SDLK_UP,SDLK_W},{SDLK_DOWN,0},
           {SDLK_LEFT,0},{SDLK_RIGHT,0},{SDLK_D,0},{SDLK_C,0},{SDLK_V,0},{SDLK_V,0}},
    .pad={SDL_GAMEPAD_BUTTON_SOUTH,SDL_GAMEPAD_BUTTON_EAST,
          SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
          SDL_GAMEPAD_BUTTON_START,SDL_GAMEPAD_BUTTON_BACK,
          SDL_GAMEPAD_BUTTON_DPAD_UP,SDL_GAMEPAD_BUTTON_DPAD_DOWN,
          SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
          SDL_GAMEPAD_BUTTON_NORTH,SDL_GAMEPAD_BUTTON_WEST,SDL_GAMEPAD_BUTTON_RIGHT_STICK,
          SDL_GAMEPAD_BUTTON_NORTH}
};
static int sActionActive, sUiActive;
int PortActionCombatActive(void){return sActionActive;}
int PortUiActive(void){return sUiActive;}
uint32_t PortFrameCount(void){return 0;}
int PortRollbackResimulating(void){return 0;} /* netplay link is outside this input fixture */
int VsBattleLocalFacingLeft(void){return 0;}
/* the game's side port_input.c reaches (the battle's card scroll, the zoom hotkeys): outside a battle here */
Mode* gCurrentMode;
Mode gModeBattle, gModeVsbattle;
void PortUiZoomStep(int dir){(void)dir;}
void PortLog(const char* fmt,...){(void)fmt;}
static void key(SDL_Keycode code,int down){
    SDL_Event e={0};e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
    e.key.key=code;PortInputHandleEvent(&e);
}
int main(void){
    assert(SDL_Init(SDL_INIT_GAMEPAD|SDL_INIT_EVENTS));
    const SDL_Keycode codes[]={SDLK_X,SDLK_Z,SDLK_A,SDLK_S,SDLK_RETURN,
        SDLK_BACKSPACE,SDLK_UP,SDLK_DOWN,SDLK_LEFT,SDLK_RIGHT,SDLK_D,SDLK_C,SDLK_V};
    const uint16_t bits[]={1,2,0x200,0x100,8,4,0x40,0x80,0x20,0x10,
        PORT_KEY_STOCK,PORT_KEY_DODGE,PORT_KEY_UNSTOCK};
    for(int i=0;i<13;i++){
        key(codes[i],1);assert(PortInputKeys()==bits[i]);
        PortInputResolve(PortInputKeys());
        key(codes[i],0);assert(!PortInputKeys());
    }
    gPortConfig.unstockButton=0;
    key(SDLK_V,1);assert(!PortInputKeys());key(SDLK_V,0);
    for(int i=0;i<60;i++) {PortInputResolve(PORT_KEY_UNSTOCK);assert(!PortTakeUnstock());}
    gPortConfig.unstockButton=1;PortInputResolve(0);
    key(SDLK_LEFT,1);key(SDLK_RIGHT,1);assert(!PortInputKeys());
    PortInputResolve(PortInputKeys());
    key(SDLK_RIGHT,0);assert(PortInputKeys()==0x20);key(SDLK_LEFT,0);
    assert((PortInputResolve(PORT_KEY_STOCK)&0x300)==0x300);
    PortInputResolve(PORT_KEY_DODGE);
    assert(PortTakeDodge(KEY_LEFT,0)==KEY_LEFT);
    PortInputResolve(0);PortInputResolve(PORT_KEY_DODGE);
    for(int i=0;i<29;i++){assert(!PortInputResolve(PORT_KEY_UNSTOCK));assert(!PortTakeUnstock());}
    PortInputResolve(PORT_KEY_UNSTOCK);assert(PortTakeUnstock());assert(!PortTakeUnstock());
    for(int i=0;i<60;i++){PortInputResolve(PORT_KEY_UNSTOCK);assert(!PortTakeUnstock());}
    PortInputResolve(0);
    for(int i=0;i<30;i++)PortInputResolve(PORT_KEY_UNSTOCK);
    key(SDLK_X,1);
    SDL_Event lost={0};lost.type=SDL_EVENT_WINDOW_FOCUS_LOST;PortInputHandleEvent(&lost);
    assert(!PortInputKeys() && !PortTakeDodge(0,0) && !PortTakeUnstock());
    for(int i=0;i<30;i++)PortInputResolve(PORT_KEY_UNSTOCK);
    PortInputSuppressUntilRelease();assert(!PortTakeUnstock());assert(!PortInputKeys());
    /* Rebinding while a key is down cannot leave the old action stuck: its
       later release no longer matches the new mapping. */
    key(SDLK_X,1);assert(PortInputKeys()==KEY_A);
    gPortConfig.keys[PORT_ACT_A][0]=SDLK_Q;
    key(SDLK_X,0);assert(PortInputKeys()==KEY_A);
    PortInputResetBindings();assert(!PortInputKeys());
    key(SDLK_Q,1);assert(PortInputKeys()==KEY_A);
    PortInputResolve(PortInputKeys());
    key(SDLK_Q,0);assert(!PortInputKeys());
    gPortConfig.keys[PORT_ACT_A][0]=SDLK_X;
    /* A key released while the overlay owns key-up must not keep suppression
       latched forever after closing. A fresh press must work immediately. */
    key(SDLK_X,1);assert(PortInputKeys()==KEY_A);
    PortInputSuppressUntilRelease();assert(!PortInputKeys());
    key(SDLK_Z,1);assert(PortInputKeys()==KEY_B);
    PortInputResolve(PortInputKeys());
    key(SDLK_Z,0);assert(!PortInputKeys());
    /* Three physical keyboard-event taps must reach three fresh A edges;
       key repeat while held cannot fabricate an extra attack request. The
       native combat suite separately checks how the actor consumes them. */
    sActionActive=1;PortInputResolve(0);
    unsigned previous=0,attackEdges=0;
    for(int i=0;i<3;i++) {
        key(SDLK_X,1);
        unsigned held=PortInputResolve(PortInputKeys());
        attackEdges+=((held&~previous)&KEY_A)!=0;previous=held;
        SDL_Event again={0};again.type=SDL_EVENT_KEY_DOWN;again.key.key=SDLK_X;again.key.repeat=true;
        PortInputHandleEvent(&again);held=PortInputResolve(PortInputKeys());
        assert(!(held&~previous&KEY_A));previous=held;
        key(SDLK_X,0);held=PortInputResolve(PortInputKeys());
        assert(!(held&KEY_A));previous=held;
    }
    assert(attackEdges==3);
    /* A down AND up before the game samples a frame must not disappear.
       Multiple UI reads must not consume it; the real frame sample does. */
    attackEdges=0;previous=0;
    for(int i=0;i<3;i++) {
        key(SDLK_X,1);key(SDLK_X,0);
        assert(PortInputKeys()==KEY_A && PortInputKeys()==KEY_A);
        unsigned held=PortInputResolve(PortInputKeys());
        attackEdges+=((held&~previous)&KEY_A)!=0;previous=held;
        assert(PortInputKeys()==0);
        held=PortInputResolve(PortInputKeys());assert(!held);previous=held;
    }
    assert(attackEdges==3);
    key(SDLK_X,1);key(SDLK_X,0);PortInputHandleEvent(&lost);
    assert(!PortInputKeys());PortInputResolve(0);
    key(SDLK_X,1);key(SDLK_X,0);PortInputResetBindings();
    assert(!PortInputKeys());PortInputResolve(0);
    key(SDLK_X,1);key(SDLK_X,0);PortInputSuppressUntilRelease();
    assert(!PortInputKeys());
    /* A genuinely fresh press after overlay closure is accepted immediately. */
    key(SDLK_X,1);key(SDLK_X,0);assert(PortInputResolve(PortInputKeys())==KEY_A);
    assert(!PortInputResolve(PortInputKeys()));sActionActive=0;
    /* Guard uses the production keyboard event/resolve path, independently
       of the experimental Unstock option and without emitting L/R magic. */
    sActionActive=1;gPortConfig.unstockButton=0;PortInputResolve(0);
    key(SDLK_V,1);assert(PortInputKeys()==PORT_KEY_GUARD);
    assert(PortInputResolve(PortInputKeys())==0 && PortActionGuardHeld());
    SDL_Event repeat={0};repeat.type=SDL_EVENT_KEY_DOWN;repeat.key.key=SDLK_V;repeat.key.repeat=true;
    PortInputHandleEvent(&repeat);
    for(int i=0;i<60;i++)assert(!PortInputResolve(PortInputKeys()) && PortActionGuardHeld());
    key(SDLK_V,0);PortInputResolve(PortInputKeys());assert(!PortActionGuardHeld());
    key(SDLK_V,1);PortInputResolve(PortInputKeys());assert(PortActionGuardHeld());
    PortInputHandleEvent(&lost);assert(!PortActionGuardHeld());
    PortInputResolve(PortInputKeys());assert(!PortActionGuardHeld());
    key(SDLK_V,1);PortInputResolve(PortInputKeys());assert(PortActionGuardHeld());
    sUiActive=1;assert(!PortActionGuardHeld());sUiActive=0;
    PortInputSuppressUntilRelease();assert(!PortActionGuardHeld());assert(!PortInputKeys());
    PortInputResolve(0);
    gPortConfig.keys[PORT_ACT_GUARD][0]=SDLK_F;
    key(SDLK_V,1);PortInputResolve(PortInputKeys());assert(!PortActionGuardHeld());key(SDLK_V,0);
    key(SDLK_F,1);PortInputResolve(PortInputKeys());assert(PortActionGuardHeld());
    key(SDLK_F,0);PortInputResolve(PortInputKeys());assert(!PortActionGuardHeld());
    gPortConfig.keys[PORT_ACT_GUARD][0]=SDLK_V;sActionActive=0;gPortConfig.unstockButton=1;
    SDL_VirtualJoystickDesc desc;SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;desc.name="ReKHCoM controls test";
    SDL_JoystickID id=SDL_AttachVirtualJoystick(&desc);assert(id);
    SDL_Joystick* joystick=SDL_OpenJoystick(id);assert(joystick);
    SDL_Gamepad* pad=SDL_OpenGamepad(id);assert(pad);
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,true));
    SDL_UpdateJoysticks();key(SDLK_LEFT,1);assert(PortInputKeys()==(1|0x20));
    PortInputResolve(PortInputKeys());
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,false));
    SDL_UpdateJoysticks();key(SDLK_LEFT,0);assert(!PortInputKeys());
    /* A real SDL virtual North button holds guard and never also casts,
       cycles, or takes the legacy Stock-OFF attack fallback. */
    sActionActive=1;PortInputResolve(0);
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_NORTH,true));
    SDL_UpdateJoysticks();assert(PortInputKeys()==PORT_KEY_GUARD);
    assert(!PortInputResolve(PortInputKeys()) && PortActionGuardHeld());
    gPortConfig.stockButton=0;
    assert(PortInputKeys()==PORT_KEY_GUARD);
    assert(!PortInputResolve(PortInputKeys()) && PortActionGuardHeld());
    PortInputHandleEvent(&lost);assert(!PortActionGuardHeld());
    PortInputResolve(PortInputKeys());assert(!PortActionGuardHeld());
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_NORTH,false));
    SDL_UpdateJoysticks();PortInputResolve(PortInputKeys());assert(!PortActionGuardHeld());
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_NORTH,true));
    SDL_UpdateJoysticks();PortInputResolve(PortInputKeys());assert(PortActionGuardHeld());
    sActionActive=0;gPortConfig.stockButton=1;
    assert(!PortActionGuardHeld());assert(PortInputKeys()==PORT_KEY_STOCK);
    assert((PortInputResolve(PortInputKeys())&0x300)==0x300);
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_NORTH,false));
    SDL_UpdateJoysticks();PortInputResolve(PortInputKeys());
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_RIGHT_STICK,true));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==PORT_KEY_UNSTOCK);
    gPortConfig.unstockButton=0;assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_RIGHT_STICK,false));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,0));
    gPortConfig.pad[PORT_ACT_L]=PORT_PAD_AXIS_BASE+SDL_GAMEPAD_AXIS_LEFT_TRIGGER*2;
    gPortConfig.pad[PORT_ACT_R]=PORT_PAD_AXIS_BASE+SDL_GAMEPAD_AXIS_RIGHT_TRIGGER*2;
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-32768));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768));
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-2000));
    SDL_UpdateJoysticks();assert(!PortInputKeys()); /* below gamepad threshold */
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_L);
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==(KEY_L|KEY_R));
    PortInputSuppressUntilRelease();assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-32768));
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768));
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_R);
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768));
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    /* A disconnected controller cannot leave a remapped action held. */
    assert(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,32767));
    SDL_UpdateJoysticks();assert(PortInputKeys()==KEY_L);
    SDL_CloseGamepad(pad);SDL_CloseJoystick(joystick);SDL_DetachVirtualJoystick(id);
    SDL_UpdateJoysticks();assert(!PortInputKeys());
    SDL_Quit();puts("Input: three real X down/up event edges without repeat attacks; 13 classic keys; Guard V/remap/release/focus/overlay; SDL virtual North without cast/cycle/Stock-OFF attack; classic stock, dodge and triggers PASS");
    return 0;
}
