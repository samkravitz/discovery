#include <stdint.h>

#include "instruction.h"
#include "common.h"

bool isConditionMet(Instruction instruction, arm &cpu) {
    uint8_t nFlag = instruction >> 31 & 0x1;
    uint8_t zFlag = instruction >> 30 & 0x1;
    uint8_t cFlag = instruction >> 29 & 0x1;
    uint8_t vFlag = instruction >> 28 & 0x1; 

    return nFlag == cpu.getConditionCodeFlag(N) &&
        zFlag == cpu.getConditionCodeFlag(Z) &&
        cFlag == cpu.getConditionCodeFlag(C) &&
        vFlag == cpu.getConditionCodeFlag(V);
}