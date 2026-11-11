#ifndef PORT_ACTION_H
#define PORT_ACTION_H

/* Experimental, ROM-independent action controller. This owns intent, input
 * buffering and resources only; the game adapter owns animations and damage.
 * Tick exactly once per simulation frame, including frames with a busy actor.
 * Peek is read-only. Commit only after the engine accepts the returned intent.
 * Never tick from a renderer, or use this in link battles or tutorials. */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum PortActionButton {
    PORT_ACTION_ATTACK = 1u << 0,
    PORT_ACTION_SPECIAL = 1u << 1,
    PORT_ACTION_MAGIC = 1u << 2,
    PORT_ACTION_PREV_MAGIC = 1u << 3,
    PORT_ACTION_NEXT_MAGIC = 1u << 4,
    PORT_ACTION_UP = 1u << 5,
    PORT_ACTION_DOWN = 1u << 6,
    PORT_ACTION_LEFT = 1u << 7,
    PORT_ACTION_RIGHT = 1u << 8,
    PORT_ACTION_MODIFIER = 1u << 9,
    PORT_ACTION_ASSIST = 1u << 10,
    PORT_ACTION_PREV_ASSIST = 1u << 11,
    PORT_ACTION_NEXT_ASSIST = 1u << 12,
    PORT_ACTION_PREV_WEAPON = 1u << 13,
    PORT_ACTION_NEXT_WEAPON = 1u << 14
};

enum PortActionCommand {
    PORT_ACTION_NONE,
    PORT_ACTION_GROUND_1,
    PORT_ACTION_GROUND_2,
    PORT_ACTION_GROUND_3,
    PORT_ACTION_AIR_1,
    PORT_ACTION_AIR_2,
    PORT_ACTION_AIR_3,
    PORT_ACTION_SLIDING_DASH,
    PORT_ACTION_STRIKE_RAID,
    PORT_ACTION_BLITZ,
    PORT_ACTION_STUN_IMPACT,
    PORT_ACTION_SONIC_BLADE,
    PORT_ACTION_RAGNAROK,
    PORT_ACTION_FIRE,
    PORT_ACTION_BLIZZARD,
    PORT_ACTION_THUNDER,
    PORT_ACTION_CURE,
    PORT_ACTION_GRAVITY,
    PORT_ACTION_STOP,
    PORT_ACTION_AERO,
    PORT_ACTION_NATIVE_TECHNIQUE,
    PORT_ACTION_COMMAND_COUNT
};
#define PORT_ACTION_NO_NATIVE_MOVE UINT32_MAX

enum {
    PORT_ACTION_MAGIC_COUNT = 7,
    PORT_ACTION_MAX_MP = 100,
    PORT_ACTION_BUFFER_FRAMES = 12,
    PORT_ACTION_BUFFER_MAX_FRAMES = 42,
    PORT_ACTION_COMBO_FRAMES = 36,
    PORT_ACTION_DEFENSE_COMBO_FRAMES = 54,
    PORT_ACTION_REGEN_DELAY = 90,
    PORT_ACTION_REGEN_INTERVAL = 6,
    PORT_ACTION_COUNTER_FRAMES = 75,
    PORT_ACTION_DODGE_MP_REWARD = 10,
    PORT_ACTION_FINISHER_FRAMES = 36,
    PORT_ACTION_MAX_ASSIST = 100,
    PORT_ACTION_ASSIST_FAMILY_COUNT = 14,
    PORT_ACTION_CHARGE_MIDDLE = 12,
    PORT_ACTION_CHARGE_FULL = 30
};
enum PortActionDenial { PORT_ACTION_DENIAL_NONE, PORT_ACTION_DENIAL_MP,
    PORT_ACTION_DENIAL_RECHARGE, PORT_ACTION_DENIAL_COOLDOWN, PORT_ACTION_DENIAL_EFFECT };

typedef struct PortActionCommandDef {
    const char* name;
    uint16_t mpCost;
    uint16_t cooldownFrames;
} PortActionCommandDef;

typedef struct PortActionInput {
    uint16_t pressed;
    uint16_t held;
    uint8_t airborne;
    uint8_t facingLeft;
    uint8_t canAct;
    uint8_t comboGracePaused; /* an early recovery link is not the end-of-action grace period */
    uint8_t interrupted;
    uint8_t magicMenu;
    uint8_t spellBusy;
} PortActionInput;

typedef struct PortActionIntent {
    uint32_t token;
    uint16_t command;
    uint16_t mpCost;
    uint32_t nativeMove; /* Opaque native MOVE_ value; some tiered IDs use bit31. */
    uint8_t comboStep; /* 1..3 for a basic attack, 0 for other commands */
    uint8_t airborne;
    uint8_t finisher; /* extra move after three normal hits, or a direct technique */
} PortActionIntent;

typedef struct PortActionState {
    uint32_t revision;
    uint32_t selectedNativeMove;
    uint32_t bufferedNativeMove;
    uint16_t mp;
    uint16_t assistGauge;
    uint16_t cooldown[PORT_ACTION_COMMAND_COUNT];
    uint16_t regenDelay;
    uint16_t comboFrames;
    uint16_t defenseComboFrames; /* absolute continuation budget; defense cannot refresh it */
    uint16_t feedbackFrames;
    uint16_t lastCommand;
    uint16_t finisherFrames;
    uint16_t counterFrames; /* one earned finisher after a precisely timed dodge */
    uint16_t defenseFeedbackFrames;
    uint16_t magicPaletteFrames;
    uint16_t denialFrames, deniedCommand;
    uint16_t mpGainFrames;
    uint16_t bufferedHeld;
    uint8_t magicIndex;
    uint8_t recharging; /* an exhausted MP bar must fully recharge */
    uint8_t regenTick;
    uint8_t comboStep; /* count of accepted basic attacks in this chain */
    uint8_t comboAirborne;
    uint8_t bufferFrames;
    uint8_t bufferAgeFrames; /* hard lifetime, including busy actor/effect-channel waits */
    uint8_t bufferedKind;
    uint8_t bufferedMagic;
    uint8_t airborne;
    uint8_t facingLeft;
    uint8_t canAct;
    uint8_t dodgeRewarded; /* prevents repeated rewards from one roll/hitbox */
    uint8_t meleeRewardAvailable;
    uint8_t denial;
    uint8_t lastMpGain;
    uint8_t confirmedChain;
    uint8_t finisherAirborne;
    uint8_t spellBusy;
    uint8_t magicQueued;
    uint8_t assistFamily;
    uint8_t keybladeIndex;
    uint8_t attackCharging, magicCharging, assistCharging;
    uint8_t attackChargeTicks, magicChargeTicks, assistChargeTicks;
    uint8_t attackChargeModifier, magicChargeModifier, assistChargeModifier;
    uint8_t attackChargeFacing, magicChargeFacing;
    uint16_t attackChargeDirections, magicChargeDirections;
    uint8_t bufferedCharge, bufferedModifier, bufferedFacing;
    uint8_t assistQueued;
    uint8_t assistDiagnostic;
    uint32_t pendingAssistMove;
} PortActionState;

void PortActionReset(PortActionState* state);
void PortActionInterrupt(PortActionState* state);
/* Voluntary defense preserves only earned sequence position for a bounded
 * re-entry window. Damage/status interruption must still use Interrupt. */
void PortActionDefend(PortActionState* state);
void PortActionBeginDodge(PortActionState* state);
int PortActionPerfectDodge(PortActionState* state);
unsigned PortActionRestoreOnHit(PortActionState* state);
void PortActionTick(PortActionState* state, const PortActionInput* input);
int PortActionPeek(const PortActionState* state, PortActionIntent* intent);
int PortActionCommit(PortActionState* state, const PortActionIntent* intent);
const PortActionCommandDef* PortActionGetCommand(unsigned command);
unsigned PortActionMagicCommand(unsigned index);
void PortActionSelectFinisher(PortActionState* state,uint32_t nativeMove);
void PortActionQueueNative(PortActionState* state,uint32_t nativeMove);
int PortActionPendingNative(const PortActionState* state,uint32_t* nativeMove);
void PortActionAddAssistGauge(PortActionState* state,unsigned amount);
int PortActionPendingAssist(const PortActionState* state,uint32_t* nativeMove);
int PortActionCommitAssist(PortActionState* state,uint32_t nativeMove);
void PortActionQueueAssist(PortActionState* state,uint32_t nativeMove,int diagnosticFree);
uint32_t PortActionKeybladeMove(const PortActionState* state);

#ifdef __cplusplus
}
#endif
#endif
