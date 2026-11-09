#ifndef GUARD_BTL_COLLISION_H
#define GUARD_BTL_COLLISION_H

#include "battle_actor_types.h"
#include "types.h"

enum AttackResult {
    ATTACK_RESULT_NONE,
    ATTACK_RESULT_HIT,
    ATTACK_RESULT_BLOCKED
};

#ifdef PLATFORM_PC
/* Independent actions keep their damage owner while the card channel serves
 * another actor. Contexts may nest; restore the pointer returned by Set. NULL
 * uses the original card-channel ownership. Do not retain a context between
 * task updates or after destroying its actor. */
BtlObj* BtlSetAttackSource(BtlObj* source);
BtlObj* BtlGetAttackSource(void);
u8 BtlAttackIsPlayer(void);
#endif

s32 ApplyAttackBox(s32 attack, s32 x, s32 y, s32 z, s16 halfX, s16 halfY, s16 halfZ);
void ColliderPoolsInit();
void ColliderInit(Collider* collider, u32 type, u16 radius, u16 height);
void ColliderUnregister(Collider* collider);
void ColliderSetPosition(Collider* collider, s32 x, s32 y, s32 z);
void ColliderUpdateAll();
void ColliderSetDisabled(Collider* collider, u8 on);
void ColliderSetRadius(Collider* collider, u16 radius);
void ColliderSetHeight(Collider* collider, u16 height);
u8 ColliderIsTouchingType(Collider* collider, s32 bit);

struct FldObj;

u8 TestAttackBox(s32 x, s32 y, s32 z, s16 halfX, s16 halfY, s16 halfZ);
s32 ApplyAttackToBtlObj(s32 attack, BtlObj* obj);
void FldObjRegister(struct FldObj* obj);
void FldObjUnregister(struct FldObj* obj);

#endif
