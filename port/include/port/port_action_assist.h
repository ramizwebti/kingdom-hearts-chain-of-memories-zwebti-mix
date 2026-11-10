#ifndef PORT_ACTION_ASSIST_H
#define PORT_ACTION_ASSIST_H
#include <stdint.h>

enum PortActionAssistStart {
    PORT_ASSIST_INVALID = -1,
    PORT_ASSIST_WAIT = 0,
    PORT_ASSIST_STARTED = 1
};

/* Native companions run beside Sora, without changing his action or collider.
 * A waiting request owns no gauge, tiles, task, or effect channel. */
int PortActionTryStartAssist(uint32_t nativeMove);
int PortActionAssistSupported(uint32_t nativeMove);
int PortActionAssistActive(void);
uint32_t PortActionAssistMove(void);
/* Positive native companion collision only; never a range query or miss. */
void PortActionAssistContact(unsigned attack, unsigned damage);
void PortActionAssistHealed(unsigned before, unsigned after);

/* Native init adapters apply only during the synchronous assist spawn. */
void* PortActionAssistSpawnTiles(int mainSide, void* originalTiles);
int PortActionAssistSpawnOrigin(int mainSide, int32_t* x, int32_t* y, int32_t* z);
int PortActionAssistOwnsNative(const void* nativeWork);
#endif
