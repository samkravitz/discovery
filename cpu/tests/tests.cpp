#include <assert.h>
#include <iostream>
#include <bitset>

#include "../cpu.h"
#include "../common.h"

void conditionFlagTests() {
    cpu a;
    
    // assert all flags are 0 upon initialization
    assert(a.a.getConditionCodeFlag(N) == 0);
    assert(a.a.getConditionCodeFlag(Z) == 0);
    assert(a.a.getConditionCodeFlag(C) == 0);
    assert(a.a.getConditionCodeFlag(V) == 0);

    // set N, C, and V flags to 1
    a.a.setConditionCodeFlag(N, 1);
    a.a.setConditionCodeFlag(C, 1);
    a.a.setConditionCodeFlag(V, 1);

    // assert N, C, and V flags are now 1 and Z is still 0
    assert(a.a.getConditionCodeFlag(N) == 1);
    assert(a.a.getConditionCodeFlag(C) == 1);
    assert(a.a.getConditionCodeFlag(V) == 1);
    assert(a.a.getConditionCodeFlag(Z) == 0);

    // now set Z flag to 1 and assert it got changed
    a.a.setConditionCodeFlag(Z, 1);
    assert(a.a.getConditionCodeFlag(Z) == 1);

    // set N back to 0 and assert it got changed
    a.a.setConditionCodeFlag(N, 0);
    assert(a.a.getConditionCodeFlag(N) == 0);
    
    // try setting the code of a non recognized flag (should print some error message)
    a.a.setConditionCodeFlag((ConditionCodeFlag_t) 9, 1);
    // try setting the code of a non recognized bit (should print some error message)
    a.a.setConditionCodeFlag(Z, 4);
}

int main() {
    conditionFlagTests();
    return 0;
}

