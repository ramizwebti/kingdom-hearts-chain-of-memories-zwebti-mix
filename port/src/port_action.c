#include "port/port_action.h"
#include "port/port_action_routes.h"
#include <string.h>

/* Costs and cooldowns are provisional until in-game balance testing. Using
 * existing moves is the adapter's job; these IDs never encode ROM addresses. */
static const PortActionCommandDef sCommands[PORT_ACTION_COMMAND_COUNT] = {
    { "", 0, 0 },
    { "Attack", 0, 0 },
    { "Attack", 0, 0 },
    { "Attack", 0, 0 },
    { "Aerial Attack", 0, 0 },
    { "Aerial Attack", 0, 0 },
    { "Aerial Attack", 0, 0 },
    { "Sliding Dash", 12, 42 },
    { "Strike Raid", 18, 72 },
    { "Blitz", 0, 0 },
    { "Stun Impact", 22, 150 },
    { "Air Rush", 0, 0 },
    { "Ragnarok", 30, 180 },
    { "Fire", 12, 0 },
    { "Blizzard", 14, 0 },
    { "Thunder", 22, 0 },
    { "Cure", 35, 0 },
    { "Gravity", 22, 0 },
    { "Stop", 30, 0 },
    { "Aero", 24, 0 },
    { "Native Technique", 0, 0 }
};

static const uint16_t sMagic[PORT_ACTION_MAGIC_COUNT] = {
    PORT_ACTION_FIRE, PORT_ACTION_BLIZZARD, PORT_ACTION_THUNDER,
    PORT_ACTION_CURE, PORT_ACTION_GRAVITY, PORT_ACTION_STOP, PORT_ACTION_AERO
};

enum BufferedKind { BUFFER_NONE, BUFFER_ATTACK, BUFFER_MAGIC, BUFFER_NATIVE };

const PortActionCommandDef* PortActionGetCommand(unsigned command) {
    return command < PORT_ACTION_COMMAND_COUNT ? &sCommands[command] : &sCommands[0];
}

unsigned PortActionMagicCommand(unsigned index) {
    return sMagic[index % PORT_ACTION_MAGIC_COUNT];
}

void PortActionReset(PortActionState* state) {
    memset(state, 0, sizeof(*state));
    state->mp = PORT_ACTION_MAX_MP;
    state->revision = 1;
    state->selectedNativeMove=PORT_ACTION_NO_NATIVE_MOVE;
}

void PortActionSelectFinisher(PortActionState* state,uint32_t nativeMove) {
    state->selectedNativeMove=nativeMove;++state->revision;
}

void PortActionQueueNative(PortActionState* state,uint32_t nativeMove) {
    state->bufferedNativeMove=nativeMove;state->bufferedKind=BUFFER_NATIVE;
    state->bufferFrames=36;state->magicQueued=0;++state->revision;
}

void PortActionAddAssistGauge(PortActionState* state,unsigned amount) {
    unsigned value=state->assistGauge+amount;
    state->assistGauge=(uint16_t)(value>PORT_ACTION_MAX_ASSIST ? PORT_ACTION_MAX_ASSIST : value);
}
void PortActionQueueAssist(PortActionState* state,uint32_t move,int diagnosticFree) {
    state->pendingAssistMove=move;state->assistQueued=1;
    state->assistDiagnostic=diagnosticFree!=0;++state->revision;
}
int PortActionPendingAssist(const PortActionState* state,uint32_t* move) {
    if(!state->assistQueued || (!state->assistDiagnostic && state->assistGauge<PORT_ACTION_MAX_ASSIST)) return 0;
    if(move) *move=state->pendingAssistMove;
    return 1;
}
int PortActionCommitAssist(PortActionState* state,uint32_t move) {
    uint32_t pending;
    if(!PortActionPendingAssist(state,&pending) || move!=pending) return 0;
    if(!state->assistDiagnostic) state->assistGauge-=PORT_ACTION_MAX_ASSIST;
    state->assistQueued=state->assistDiagnostic=0;++state->revision;return 1;
}
uint32_t PortActionKeybladeMove(const PortActionState* state) {
    return PortActionRouteKeyblade(state->keybladeIndex);
}

int PortActionPendingNative(const PortActionState* state,uint32_t* nativeMove) {
    if (!state->bufferFrames) return 0;
    if (state->bufferedKind==BUFFER_NATIVE) {
        if (nativeMove) *nativeMove=state->bufferedNativeMove;
    } else if (state->bufferedKind==BUFFER_MAGIC &&
            (state->bufferedCharge>=PORT_ACTION_CHARGE_MIDDLE || state->bufferedModifier)) {
        if (nativeMove) *nativeMove=state->bufferedNativeMove;
    } else return 0;
    return 1;
}

void PortActionInterrupt(PortActionState* state) {
    state->revision++;
    state->bufferFrames = 0;
    state->bufferedKind = BUFFER_NONE;
    state->comboStep = 0;
    state->comboFrames = 0;
    state->canAct = 0;
    state->lastCommand = PORT_ACTION_NONE;
    state->feedbackFrames = 0;
    state->counterFrames = 0;
    state->defenseFeedbackFrames = 0;
    state->meleeRewardAvailable = 0;
    state->magicPaletteFrames = 0;
    state->denialFrames = state->denial = 0;
    state->finisherFrames=0;
    state->confirmedChain=0;
    state->magicQueued=0;
    state->attackCharging=state->magicCharging=state->assistCharging=0;
}

/* The game adapter calls this only when a new roll has actually started. */
void PortActionBeginDodge(PortActionState* state) {
    state->dodgeRewarded = 0;
}

/* Only a geometrically overlapping live enemy attack can earn this reward.
 * It does not alter invulnerability, enemy damage or the dodge duration. */
int PortActionPerfectDodge(PortActionState* state) {
    unsigned mp;
    if (state->dodgeRewarded) return 0;
    state->dodgeRewarded = 1;
    state->revision++;
    state->counterFrames = PORT_ACTION_COUNTER_FRAMES;
    state->defenseFeedbackFrames = 45;
    mp = state->mp + PORT_ACTION_DODGE_MP_REWARD;
    state->mp = mp < PORT_ACTION_MAX_MP ? (uint16_t)mp : PORT_ACTION_MAX_MP;
    if (state->mp == PORT_ACTION_MAX_MP) state->recharging = 0;
    PortActionAddAssistGauge(state,8);
    return 1;
}

unsigned PortActionRestoreOnHit(PortActionState* state) {
    unsigned before=state->mp, reward, step;
    if (!state->meleeRewardAvailable) return 0;
    state->meleeRewardAvailable=0;
    step=state->lastCommand==PORT_ACTION_BLITZ || state->lastCommand==PORT_ACTION_SONIC_BLADE ? 3 :
        state->lastCommand>=PORT_ACTION_GROUND_1 && state->lastCommand<=PORT_ACTION_AIR_3 ?
        (state->lastCommand-PORT_ACTION_GROUND_1)%3+1 : 0;
    if (!step) return 0;
    PortActionAddAssistGauge(state,step==3 ? 10 : step==2 ? 8 : 6);
    if(state->confirmedChain==step-1) state->confirmedChain=step;
    else state->confirmedChain=step==1 ? 1 : 0;
    /* All three normal swings animate even on a miss. Their contacts award
     * MP; the separate fourth-input finishing move has no hit/progression gate. */
    reward=step==3 ? 5 : 1;
    state->mp=state->mp+reward>PORT_ACTION_MAX_MP ? PORT_ACTION_MAX_MP : state->mp+reward;
    if (state->mp>before) { state->lastMpGain=state->mp-before; state->mpGainFrames=36; }
    if (state->mp==PORT_ACTION_MAX_MP) state->recharging=0;
    ++state->revision;
    return state->mp-before;
}

void PortActionTick(PortActionState* state, const PortActionInput* input) {
    unsigned i;
    state->revision++;
    for (i = 0; i < PORT_ACTION_COMMAND_COUNT; i++) {
        if (state->cooldown[i]) state->cooldown[i]--;
    }
    if (state->feedbackFrames) state->feedbackFrames--;
    if (state->counterFrames) state->counterFrames--;
    if (state->defenseFeedbackFrames) state->defenseFeedbackFrames--;
    if (state->denialFrames) --state->denialFrames;
    if (state->mpGainFrames) --state->mpGainFrames;
    state->magicPaletteFrames=0;
    if (state->finisherFrames && !--state->finisherFrames) state->confirmedChain=0;
    /* A combo may outlive an entire slow authored animation. Its grace
     * period starts when the actor can actually accept the next input. */
    if (state->comboFrames && input->canAct && !state->attackCharging && --state->comboFrames == 0) {
        state->comboStep=0;
        if(!state->finisherFrames) state->confirmedChain=0;
    }
    /* A visible single spell request waits for the effect channel; Attack,
     * spell selection, damage or a mode change cancels it. Do not silently
     * expire it while the game is still displaying the previous spell. */
    if (state->bufferFrames && input->canAct &&
        !((state->bufferedKind==BUFFER_MAGIC || state->bufferedKind==BUFFER_NATIVE ||
            (state->bufferedKind==BUFFER_ATTACK && state->comboStep==3)) && input->spellBusy) &&
        --state->bufferFrames == 0)
        state->bufferedKind = BUFFER_NONE;
    state->magicQueued=state->bufferFrames && state->bufferedKind==BUFFER_MAGIC;

    if (state->regenDelay) {
        state->regenDelay--;
        state->regenTick = 0;
    } else if (state->mp < PORT_ACTION_MAX_MP) {
        if (++state->regenTick >= PORT_ACTION_REGEN_INTERVAL) {
            unsigned mp = state->mp + (state->recharging ? 2u : 1u);
            state->mp = mp < PORT_ACTION_MAX_MP ? (uint16_t)mp : PORT_ACTION_MAX_MP;
            state->regenTick = 0;
            if (state->mp == PORT_ACTION_MAX_MP) state->recharging = 0;
        }
    } else {
        state->regenTick = 0;
        state->recharging = 0;
    }

    state->airborne = input->airborne != 0;
    state->facingLeft = input->facingLeft != 0;
    state->canAct = input->canAct != 0;
    state->spellBusy=input->spellBusy!=0;
    if ((input->pressed & (PORT_ACTION_PREV_MAGIC | PORT_ACTION_NEXT_MAGIC | PORT_ACTION_SPECIAL)) == PORT_ACTION_PREV_MAGIC)
        state->magicIndex = (state->magicIndex + PORT_ACTION_MAGIC_COUNT - 1) % PORT_ACTION_MAGIC_COUNT;
    else if (!(input->pressed&PORT_ACTION_PREV_MAGIC) &&
             (input->pressed & (PORT_ACTION_NEXT_MAGIC|PORT_ACTION_SPECIAL)))
        state->magicIndex = (state->magicIndex + 1) % PORT_ACTION_MAGIC_COUNT;
    if(input->pressed&(PORT_ACTION_PREV_MAGIC|PORT_ACTION_NEXT_MAGIC|PORT_ACTION_SPECIAL)) {
        if(state->bufferedKind==BUFFER_MAGIC) {
            state->bufferedKind=BUFFER_NONE;state->bufferFrames=0;state->magicQueued=0;
        }
    }
    if(input->pressed&PORT_ACTION_PREV_ASSIST)
        state->assistFamily=(state->assistFamily+PORT_ACTION_ASSIST_FAMILY_COUNT-1)%PORT_ACTION_ASSIST_FAMILY_COUNT;
    if(input->pressed&PORT_ACTION_NEXT_ASSIST)
        state->assistFamily=(state->assistFamily+1)%PORT_ACTION_ASSIST_FAMILY_COUNT;
    if(input->pressed&PORT_ACTION_PREV_WEAPON)
        state->keybladeIndex=(state->keybladeIndex+PortActionRouteKeybladeCount()-1)%PortActionRouteKeybladeCount();
    if(input->pressed&PORT_ACTION_NEXT_WEAPON)
        state->keybladeIndex=(state->keybladeIndex+1)%PortActionRouteKeybladeCount();

    /* Spell selection is available even during a hit reaction. It never
     * raises an action or cancels damage. */
    if (input->interrupted) {
        PortActionInterrupt(state);
        return;
    }

    if(input->pressed&PORT_ACTION_ASSIST) {
        if(state->assistGauge>=PORT_ACTION_MAX_ASSIST && !state->assistQueued) {
            state->assistCharging=1;state->assistChargeTicks=0;
            state->assistChargeModifier=(input->held&PORT_ACTION_MODIFIER)!=0;
        }
    }
    if(state->assistCharging) {
        if(state->assistChargeTicks<PORT_ACTION_CHARGE_FULL) ++state->assistChargeTicks;
        if(!(input->held&PORT_ACTION_ASSIST) || state->assistChargeTicks>=PORT_ACTION_CHARGE_FULL) {
            PortActionQueueAssist(state,PortActionRouteAssist(state->assistFamily,state->assistChargeTicks,
                state->assistChargeModifier),0);state->assistCharging=0;
        }
    }
    if(input->pressed&PORT_ACTION_MAGIC) {
        state->magicCharging=1;state->magicChargeTicks=0;
        state->magicChargeModifier=(input->held&PORT_ACTION_MODIFIER)!=0;
        state->magicChargeFacing=input->facingLeft;
        state->magicChargeDirections=input->held;
    }
    if(state->magicCharging) {
        if(state->magicChargeTicks<PORT_ACTION_CHARGE_FULL) ++state->magicChargeTicks;
        if(!(input->held&PORT_ACTION_MAGIC) || state->magicChargeTicks>=PORT_ACTION_CHARGE_FULL) {
            unsigned cost;
            state->bufferedKind=BUFFER_MAGIC;state->bufferFrames=36;
            state->bufferedMagic=state->magicIndex;state->bufferedCharge=state->magicChargeTicks;
            state->bufferedModifier=state->magicChargeModifier;state->bufferedFacing=state->magicChargeFacing;
            state->bufferedHeld=state->magicChargeDirections;
            state->bufferedNativeMove=PortActionRouteMagic(state->bufferedMagic,state->bufferedHeld,
                state->bufferedFacing,state->bufferedCharge,state->bufferedModifier);
            cost=PortActionRouteMagicCost(state->bufferedMagic,state->bufferedCharge,state->bufferedModifier);
            state->denial=state->recharging ? PORT_ACTION_DENIAL_RECHARGE : state->mp<cost ?
                PORT_ACTION_DENIAL_MP : state->spellBusy ? PORT_ACTION_DENIAL_EFFECT : PORT_ACTION_DENIAL_NONE;
            state->denialFrames=state->denial ? 45 : 0;
            if(state->recharging || state->mp<cost) { state->bufferedKind=BUFFER_NONE;state->bufferFrames=0; }
            state->magicCharging=0;
        }
    }
    if(input->pressed&PORT_ACTION_ATTACK) {
        if(state->comboStep==3 && state->comboFrames) {
            state->attackCharging=1;state->attackChargeTicks=0;
            state->attackChargeModifier=(input->held&PORT_ACTION_MODIFIER)!=0;
            state->attackChargeFacing=input->facingLeft;state->attackChargeDirections=input->held;
        } else {
            state->bufferedKind=BUFFER_ATTACK;state->bufferFrames=PORT_ACTION_BUFFER_FRAMES;
            state->bufferedHeld=input->held;state->bufferedMagic=state->magicIndex;
        }
    }
    if(state->attackCharging) {
        if(state->attackChargeTicks<PORT_ACTION_CHARGE_FULL) ++state->attackChargeTicks;
        if(!(input->held&PORT_ACTION_ATTACK) || state->attackChargeTicks>=PORT_ACTION_CHARGE_FULL) {
            PortActionQueueNative(state,PortActionRouteFinisher(state->magicIndex,input->airborne,
                state->attackChargeDirections,state->attackChargeFacing,state->attackChargeTicks,
                state->attackChargeModifier));state->bufferedHeld=state->attackChargeDirections;
            state->attackCharging=0;
        }
    }
    state->magicQueued=state->bufferFrames && state->bufferedKind==BUFFER_MAGIC;
}

int PortActionPeek(const PortActionState* state, PortActionIntent* intent) {
    unsigned command;
    unsigned step = 0;
    const PortActionCommandDef* def;
    memset(intent, 0, sizeof(*intent));
    if (!state->canAct || !state->bufferFrames) return 0;
    switch (state->bufferedKind) {
    case BUFFER_ATTACK:
        /* Landing/jumping between swings changes the move's physical form,
         * not whether two inputs already earned the third combo position. */
        step = state->comboFrames ? state->comboStep : 0;
        {
            step=state->counterFrames ? 3 : step+1;
            command = (state->airborne ? PORT_ACTION_AIR_1 : PORT_ACTION_GROUND_1) + step - 1;
        }
        break;
    case BUFFER_MAGIC:
        if(state->spellBusy) return 0;
        if(state->bufferedCharge>=PORT_ACTION_CHARGE_MIDDLE || state->bufferedModifier) {
            command=PORT_ACTION_NATIVE_TECHNIQUE;intent->nativeMove=state->bufferedNativeMove;
        } else command=PortActionMagicCommand(state->bufferedMagic);
        break;
    case BUFFER_NATIVE:
        command=PORT_ACTION_NATIVE_TECHNIQUE;intent->nativeMove=state->bufferedNativeMove;
        intent->finisher=1;
        break;
    default:
        return 0;
    }
    def = &sCommands[command];
    if (state->cooldown[command]) return 0;
    unsigned cost=state->bufferedKind==BUFFER_MAGIC ?
        PortActionRouteMagicCost(state->bufferedMagic,state->bufferedCharge,state->bufferedModifier) : def->mpCost;
    if (cost && (state->recharging || state->mp < cost)) return 0;
    intent->token = state->revision;
    intent->command = (uint16_t)command;
    intent->mpCost = (uint16_t)cost;
    intent->comboStep = (uint8_t)step;
    intent->airborne = state->airborne;
    return 1;
}

int PortActionCommit(PortActionState* state, const PortActionIntent* intent) {
    PortActionIntent expected;
    if (!PortActionPeek(state, &expected) || intent->token != expected.token ||
        intent->command != expected.command || intent->mpCost != expected.mpCost ||
        intent->comboStep != expected.comboStep || intent->airborne != expected.airborne ||
        intent->finisher != expected.finisher || intent->nativeMove != expected.nativeMove)
        return 0;

    state->revision++;
    state->mp -= intent->mpCost;
    state->cooldown[intent->command] = sCommands[intent->command].cooldownFrames;
    if (intent->mpCost) {
        state->regenDelay = PORT_ACTION_REGEN_DELAY;
        state->regenTick = 0;
        if (state->mp == 0) state->recharging = 1;
    }
    if (intent->comboStep) {
        if(intent->comboStep==1 || state->meleeRewardAvailable) state->confirmedChain=0;
        state->finisherFrames=0;
        state->meleeRewardAvailable=1;
        state->counterFrames = 0;
        state->comboStep = intent->comboStep;
        state->comboAirborne = intent->airborne;
        state->comboFrames = PORT_ACTION_COMBO_FRAMES;
    } else {
        state->finisherFrames=0;
        state->confirmedChain=0;
        state->meleeRewardAvailable=0;
        if (!intent->finisher && intent->mpCost && state->comboStep) {
            /* Spells weave between physical swings. They do not replace any
             * of the three normal hits or restart a playing animation. */
            state->comboFrames=PORT_ACTION_COMBO_FRAMES;
        } else {
            state->comboStep = 0;
            state->comboFrames = 0;
        }
    }
    state->lastCommand = intent->command;
    state->denial=0;state->denialFrames=0;
    if (intent->command>=PORT_ACTION_FIRE) state->magicPaletteFrames=0;
    state->feedbackFrames = 90;
    state->bufferedKind = BUFFER_NONE;
    state->bufferFrames = 0;
    state->magicQueued=0;
    return 1;
}
