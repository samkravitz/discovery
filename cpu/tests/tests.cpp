#include <assert.h>
#include <iostream>
#include <bitset>

#include "../cpu.h"
#include "../common.h"
#include "../instruction.h"

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

void instructionConditionFieldTests() {
    cpu a;
    Instruction i = 0b00000000000000000000000000000000; // 32 bits

    // a zerod cpu should match a zero'd instruction
    assert(isConditionMet(i, a.a) == true);

    i = 0b10000000000000000000000000000000; // n flag

    // n flag set with zero'd cpu should assert false
    assert(isConditionMet(i, a.a) == false);

    // set N, C, and V flags to 1
    a.a.setConditionCodeFlag(N, 1);
    a.a.setConditionCodeFlag(C, 1);
    a.a.setConditionCodeFlag(V, 1);
    
    // with cpu condition bits at 1011, should still assert false with this instruction
    assert(isConditionMet(i, a.a) == false);

    i = 0b10110000000000000000000000000000;
    // however, this instruction should be true
    assert(isConditionMet(i, a.a) == true);

    // set N, C, and V flags to 0 and Z to 1
    a.a.setConditionCodeFlag(N, 0);
    a.a.setConditionCodeFlag(C, 0);
    a.a.setConditionCodeFlag(V, 0);
    a.a.setConditionCodeFlag(Z, 1);

    // 0100 should be false with this condition
    assert(isConditionMet(i, a.a) == false);

    // but true with this one
    i = 0b01000000000000000000000000000000;
    assert(isConditionMet(i, a.a) == true);
}

int main() {
    conditionFlagTests();
    instructionConditionFieldTests();
    return 0;
}

