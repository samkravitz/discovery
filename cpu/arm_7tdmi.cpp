#include <iostream>
#include <bitset>

#include "arm_7tdmi.h"

// zero out fields
arm_7tdmi::arm_7tdmi() {    
    cpsr = 0;
    for (int i = 0; i < 16; ++i) registers[i] = 0;
    for (int i = 0; i < 5; ++i) spsr[i] = 0;
}

uint8_t arm_7tdmi::getConditionCodeFlag(ConditionCodeFlag_t flag) {
    Word shield = 0b10000000000000000000000000000000; // 32 bits
    switch (flag) {
        case N:
        case Z:
        case C:
        case V:
            return ((shield >> flag) & cpsr) == 0 ? 0 : 1;
        default:
            std::cerr << "Unrecognized condition code flag\n";
            return 0;
    }
}

void arm_7tdmi::setConditionCodeFlag(ConditionCodeFlag_t flag, uint8_t bit) {
    // bit can only be 0 or 1
    if (bit > 1) {
        std::cerr << "Error: set bit must be 0 or 1, it is: " << bit << "\n";
        return;
    }

    std::bitset<32> bs(cpsr);
    switch (flag) {
        case N:
            bs.set(31, bit);
            break;
        case Z:
            bs.set(30, bit);
            break;
        case C:
            bs.set(29, bit);
            break;
        case V:
            bs.set(28, bit);
            break;
        default:
            std::cerr << "Unrecognized condition code flag: " << flag <<"\n";
            return;
    }

    cpsr = (Word) bs.to_ulong();
}

void arm_7tdmi::execute() {}