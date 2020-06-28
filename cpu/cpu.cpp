#include "cpu.h"

// initialize mode and state of cpu
cpu::cpu() : arm() {
    state = USR;
    mode = ARM;
}

void cpu::execute(arm_instruction instruction) {
    switch (this->getState()) {
        case ARM:
            this->arm.execute(instruction);
            break;
        case THUMB:
            // this->thumb.execute(instruction);
            break;
    }
}