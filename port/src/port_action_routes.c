#include "port/port_action_routes.h"
#include "port/port_action.h"
#include "card_ids.h"

/* Direction is relative to Sora for horizontal branches; depth up/down keeps
 * the camera's ordinary left-stick meaning. All results are authored moves. */
enum Branch { NEUTRAL, TOWARD, AWAY, UP, DOWN };
static unsigned BranchOf(unsigned held,unsigned left) {
    if(held&PORT_ACTION_UP) return UP;
    if(held&PORT_ACTION_DOWN) return DOWN;
    if(held&PORT_ACTION_LEFT) return left ? TOWARD : AWAY;
    if(held&PORT_ACTION_RIGHT) return left ? AWAY : TOWARD;
    return NEUTRAL;
}
uint32_t PortActionRouteFinisher(unsigned magic,unsigned airborne,unsigned held,
        unsigned left,unsigned charge,unsigned elemental) {
    static const uint32_t raids[7]={MOVE_FIRE_RAID,MOVE_BLIZZARD_RAID,MOVE_THUNDER_RAID,
        MOVE_JUDGMENT,MOVE_GRAVITY_RAID,MOVE_STOP_RAID,MOVE_REFLECT_RAID};
    unsigned branch=BranchOf(held,left),charged=charge>=PORT_ACTION_CHARGE_MIDDLE;
    magic%=7;
    if(elemental) {
        if(charged && magic==0) return MOVE_FIRAGA_BREAK;
        if(charged && magic==2) return MOVE_SHOCK_IMPACT;
        if(charged && magic==3) return MOVE_TRINITY_LIMIT;
        return raids[magic];
    }
    if(airborne) return MOVE_SONIC_BLADE;
    if(branch==UP) return MOVE_RAGNAROK;
    if(branch==DOWN) return MOVE_STUN_IMPACT;
    if(branch==TOWARD) return charged ? MOVE_SONIC_BLADE : MOVE_SLIDING_DASH;
    if(branch==AWAY) return charged ? MOVE_ZANTETSUKEN : MOVE_STRIKE_RAID;
    return charged ? MOVE_ARS_ARCANUM : MOVE_BLITZ;
}
uint32_t PortActionRouteMagic(unsigned magic,unsigned held,unsigned left,
        unsigned charge,unsigned advanced) {
    static const uint32_t spells[7][3]={
        {MOVE_FIRE,MOVE_FIRA,MOVE_FIRAGA},{MOVE_BLIZZARD,MOVE_BLIZZARA,MOVE_BLIZZAGA},
        {MOVE_THUNDER,MOVE_THUNDARA,MOVE_THUNDAGA},{MOVE_CURE,MOVE_CURA,MOVE_CURAGA},
        {MOVE_GRAVITY,MOVE_GRAVIRA,MOVE_GRAVIGA},{MOVE_STOP,MOVE_STOPRA,MOVE_STOPGA},
        {MOVE_AERO,MOVE_AERORA,MOVE_AEROGA}};
    unsigned branch=BranchOf(held,left);
    magic%=7;
    if(advanced) switch(magic) {
    case 0: return branch==UP || branch==DOWN ? MOVE_MEGA_FLARE : MOVE_HOMING_FIRA;
    case 1: return branch==UP || branch==DOWN ? MOVE_AQUA_SPLASH : MOVE_HOMING_BLIZZARA;
    case 2: return MOVE_THUNDAGA;
    case 3: return branch==UP || branch==DOWN ? MOVE_HOLY : MOVE_GIFTED_MIRACLE;
    case 4:
        if(branch==UP) return MOVE_QUAKE;
        if(branch==DOWN) return MOVE_WARP;
        if(branch==AWAY) return MOVE_WARPINATOR;
        return MOVE_SYNCHRO;
    case 5:
        if(branch==UP) return MOVE_TERROR;
        if(branch==DOWN) return MOVE_TELEPORT;
        if(branch==AWAY) return MOVE_CONFUSE;
        return MOVE_BIND;
    default:return MOVE_TORNADO;
    }
    return spells[magic][charge>=PORT_ACTION_CHARGE_FULL ? 2 : charge>=PORT_ACTION_CHARGE_MIDDLE ? 1 : 0];
}
uint32_t PortActionRouteAssist(unsigned family,unsigned charge,unsigned special) {
    static const uint32_t moves[14][3]={
        {MOVE_SIMBA,MOVE_PROUD_ROAR_PAIR,MOVE_PROUD_ROAR},
        {MOVE_GENIE,MOVE_SHOWTIME_PAIR,MOVE_SHOWTIME},
        {MOVE_BAMBI,MOVE_PARADISE_PAIR,MOVE_PARADISE},
        {MOVE_DUMBO,MOVE_SPLASH_PAIR,MOVE_SPLASH},
        {MOVE_TINKER_BELL,MOVE_TWINKLE_PAIR,MOVE_TWINKLE},
        {MOVE_MUSHU,MOVE_FLARE_BREATH_PAIR,MOVE_FLARE_BREATH},
        {MOVE_CLOUD,MOVE_CROSS_SLASH,MOVE_OMNISLASH},
        {MOVE_DONALD_DUCK,MOVE_MAGIC_PAIR,MOVE_MAGIC},
        {MOVE_GOOFY,MOVE_GOOFY_CHARGE,MOVE_GOOFY_TORNADO},
        {MOVE_ALADDIN,MOVE_SANDSTORM_PAIR,MOVE_SANDSTORM},
        {MOVE_ARIEL,MOVE_SPIRAL_WAVE_PAIR,MOVE_SPIRAL_WAVE},
        {MOVE_JACK,MOVE_SURPRISE_PAIR,MOVE_SURPRISE},
        {MOVE_PETER_PAN,MOVE_HUMMINGBIRD_PAIR,MOVE_HUMMINGBIRD},
        {MOVE_THE_BEAST,MOVE_FEROCIOUS_LUNGE_PAIR,MOVE_FEROCIOUS_LUNGE}};
    family%=14;
    if(special) {
        if(family==2) return MOVE_IDYLL_ROMP;
        if(family==6) return MOVE_CROSS_SLASH_PLUS;
        if(family==7) return MOVE_BLAZING_DONALD;
    }
    return moves[family][charge>=PORT_ACTION_CHARGE_FULL ? 2 : charge>=PORT_ACTION_CHARGE_MIDDLE ? 1 : 0];
}
static const uint32_t sKeyblades[]={MOVE_KINGDOM_KEY,MOVE_OLYMPIA,MOVE_THREE_WISHES,
    MOVE_CRABCLAW,MOVE_PUMPKINHEAD,MOVE_FAIRY_HARP,MOVE_WISHING_STAR,MOVE_SPELLBINDER,
    MOVE_METAL_CHOCOBO,MOVE_LIONHEART,MOVE_LADY_LUCK,MOVE_DIVINE_ROSE,MOVE_OATHKEEPER,
    MOVE_OBLIVION,MOVE_ULTIMA_WEAPON,MOVE_DIAMOND_DUST,MOVE_ONE_WINGED_ANGEL};
unsigned PortActionRouteKeybladeCount(void) { return sizeof(sKeyblades)/sizeof(sKeyblades[0]); }
uint32_t PortActionRouteKeyblade(unsigned index) { return sKeyblades[index%PortActionRouteKeybladeCount()]; }
unsigned PortActionRouteMagicCost(unsigned magic,unsigned charge,unsigned advanced) {
    static const unsigned costs[7]={12,14,22,35,22,30,24};
    unsigned base=costs[magic%7];
    return advanced ? base+12 : charge>=PORT_ACTION_CHARGE_FULL ? base+10 :
        charge>=PORT_ACTION_CHARGE_MIDDLE ? base+5 : base;
}
const char* PortActionRouteAssistName(unsigned family) {
    static const char* const names[14]={"Simba","Genie","Bambi","Dumbo","Tinker Bell","Mushu","Cloud",
        "Donald","Goofy","Aladdin","Ariel","Jack","Peter Pan","Beast"};
    return names[family%14];
}
