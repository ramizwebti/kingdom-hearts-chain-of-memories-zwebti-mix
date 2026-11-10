#include "port/port_action_feedback.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned i;
    const PortActionFeedback* f = PortActionFeedbackGet();
    PortActionFeedbackReset();
    PortActionFeedbackHit(0);
    assert(!f->hits && !f->frames);
    PortActionFeedbackHit(5); PortActionFeedbackHit(7);
    assert(f->hits==2 && f->damage==12 && f->best==2);
    for(i=0;i<119;++i) PortActionFeedbackTick();
    assert(f->hits==2 && f->frames==1 && !f->pulse);
    PortActionFeedbackHit(3);
    assert(f->hits==3 && f->frames==120);
    for(i=0;i<120;++i) PortActionFeedbackTick();
    assert(!f->hits && !f->damage && f->best==3);
    for(i=0;i<2000;++i) PortActionFeedbackHit(UINT32_MAX);
    assert(f->hits==999 && f->damage==UINT32_MAX);
    PortActionFeedbackHurt();
    assert(!f->hits && !f->frames && !f->damage && f->best==999);
    PortActionFeedbackReset();
    assert(!f->best);
    puts("confirmed hit chain: timeout, refresh, damage break, saturation and reset passed");
    return 0;
}
