#include "port/port_action_feedback.h"
#include <string.h>
static PortActionFeedback sFeedback;
void PortActionFeedbackReset(void) { memset(&sFeedback, 0, sizeof(sFeedback)); }
void PortActionFeedbackHurt(void) {
    sFeedback.hits = sFeedback.frames = sFeedback.pulse = 0;
    sFeedback.damage = 0;
}
void PortActionFeedbackTick(void) {
    if (sFeedback.pulse) --sFeedback.pulse;
    if (sFeedback.frames && !--sFeedback.frames) PortActionFeedbackHurt();
}
void PortActionFeedbackHit(unsigned damage) {
    if (!damage) return;
    if (sFeedback.hits < 999) ++sFeedback.hits;
    if (sFeedback.hits > sFeedback.best) sFeedback.best = sFeedback.hits;
    sFeedback.damage = damage > UINT32_MAX-sFeedback.damage ? UINT32_MAX : sFeedback.damage+damage;
    sFeedback.frames = PORT_ACTION_CHAIN_FRAMES;
    sFeedback.pulse = 10;
}
const PortActionFeedback* PortActionFeedbackGet(void) { return &sFeedback; }
