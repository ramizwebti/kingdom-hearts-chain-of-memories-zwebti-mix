#ifndef PORT_ACTION_FEEDBACK_H
#define PORT_ACTION_FEEDBACK_H
#include <stdint.h>
/* Confirmed damage feedback, separate from the input/combo controller.
 * Mutation belongs exclusively to simulation; rendering reads this snapshot. */
typedef struct PortActionFeedback {
    uint16_t hits, best, frames, pulse;
    uint32_t damage;
} PortActionFeedback;
enum { PORT_ACTION_CHAIN_FRAMES = 120 };
void PortActionFeedbackReset(void);
void PortActionFeedbackTick(void);
void PortActionFeedbackHit(unsigned damage);
void PortActionFeedbackHurt(void);
const PortActionFeedback* PortActionFeedbackGet(void);
#endif
