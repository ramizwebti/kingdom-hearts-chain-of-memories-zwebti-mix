#ifndef PORT_ACTION_ENEMY_H
#define PORT_ACTION_ENEMY_H

#include "battle_actor_types.h"

/* The first independent crowd attacker uses its own authored Shadow AI.
 * Bosses, human opponents and spellcasters retain their original channels. */
int PortActionShadowIndependent(const BtlObj* actor);

#endif
