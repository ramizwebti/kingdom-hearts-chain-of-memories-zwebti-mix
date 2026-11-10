#ifndef PORT_ACTION_GAME_H
#define PORT_ACTION_GAME_H
#include "port/port_action.h"
/* Read-only simulation snapshot for native rendering; NULL outside action battles. */
const PortActionState* PortActionGameState(void);
int PortActionCombatRules(void);
int PortActionCombatRequested(void);
int PortActionCombatActive(void);
/* Capture short button edges before actor hitstop skips its update. */
void PortActionCaptureInput(void);
int PortActionPlayerIndependent(void);
int PortActionLockMeleeTarget(void);
void PortActionPlayerMeleeHit(void);
/* Held physical guard is directional and never protects from magic/back hits. */
int PortActionPlayerGuarding(void);
int PortActionGuardFaces(int32_t sourceX, int32_t sourceY);
void PortActionGuardBlocked(unsigned attack);
int PortActionSelectNativeMove(uint32_t nativeMove);
uint32_t PortActionSelectedNativeMove(void);
uint32_t PortActionSelectedFinisherMove(void);
int PortActionRequestNativeMove(uint32_t nativeMove);
int PortActionPlayerFinisherActive(void);
void PortActionPlayerImpact(unsigned attack);
void PortActionStatusContact(unsigned attack, unsigned targetKind, unsigned duration);
/* Notify only for a live enemy attack, never an AI range/sensing query. */
void PortActionObserveEnemyAttack(int32_t x, int32_t y, int32_t z,
                                 int16_t halfX, int16_t halfY, int16_t halfZ);
#endif
