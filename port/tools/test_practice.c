#include "port/port_practice.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    static const char* names[5]={"training","crowd","mixed","axel","jafar"};
    static const int entries[5]={206,0,4,186,204};
    unsigned i,j;
    assert(!PortPracticeBegin(NULL)&&!PortPracticeActive());
    assert(!PortPracticeBegin("invalid")&&!PortPracticeActive());
    for(i=0;i<5;++i) {
        assert(PortPracticeBegin(names[i])&&PortPracticeActive());
        assert(PortPracticeEntry()==entries[i]&&PortPracticeSelection()==i);
        assert(!PortPracticeMenuInput(PORT_PRACTICE_START));
        PortPracticeEnterMenu();
        assert(PortPracticeMenuInput(0)==PORT_PRACTICE_START);
        PortPracticeLaunch();
        assert(!PortPracticeMenuInput(PORT_PRACTICE_DOWN));
        PortPracticeEnterMenu();
        assert(!PortPracticeMenuInput(0)); /* a returned menu never auto-restarts */
        for(j=0;j<5;++j) PortPracticeMenuInput(PORT_PRACTICE_DOWN);
        assert(PortPracticeSelection()==i);
        PortPracticeMenuInput(PORT_PRACTICE_UP);
        assert(PortPracticeSelection()==(i+4)%5);
        PortPracticeMenuInput(PORT_PRACTICE_DOWN);
        assert(PortPracticeSelection()==i);
        PortPracticeMenuInput(PORT_PRACTICE_UP|PORT_PRACTICE_DOWN);
        assert(PortPracticeSelection()==i);
        assert(PortPracticeMenuInput(PORT_PRACTICE_START)==PORT_PRACTICE_START);
        assert(PortPracticeMenuInput(PORT_PRACTICE_BACK|PORT_PRACTICE_START)==PORT_PRACTICE_BACK);
        assert(strlen(PortPracticeName())>0);
        PortPracticeLeave();
        assert(!PortPracticeActive()&&!PortPracticeMenuInput(PORT_PRACTICE_START));
    }
    assert(PortPracticeBegin("training"));
    assert(!PortPracticeBegin("")&&!PortPracticeActive());
    puts("practice lifecycle: five presets, automatic first entry, retry, selection, exit and invalid input passed");
    return 0;
}
