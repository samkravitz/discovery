#include "cpu.h"

// initialize mode and state of cpu
cpu::cpu() : arm() {
    mode = USR;
    state = ARM;
}

void cpu::execute(Instruction instruction) {
    switch (this->getState()) {
        case ARM:
            this->arm.execute(instruction);
            break;
        case THUMB:
            // this->thumb.execute(instruction);
            break;
    }
}