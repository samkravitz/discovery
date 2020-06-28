#include <iostream>
#include <bitset>

#include "arm_7tdmi.h"
#include "arm_alu.inl"
#include "util.h"

arm_7tdmi::arm_7tdmi() {}

uint8_t arm_7tdmi::get_condition_code_flag(condition_code_flag_t flag) {
    word shield = 0b10000000000000000000000000000000; // 32 bits
    switch (flag) {
        case N:
        case Z:
        case C:
        case V:
            return ((shield >> flag) & registers.cpsr) == 0 ? 0 : 1;
        default:
            std::cerr << "Unrecognized condition code flag\n";
            return 0;
    }
}

void arm_7tdmi::set_condition_code_flag(condition_code_flag_t flag, uint8_t bit) {
    // bit can only be 0 or 1
    if (bit > 1) {
        std::cerr << "Error: set bit must be 0 or 1, it is: " << bit << "\n";
        return;
    }

    std::bitset<32> bs(registers.cpsr);
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

    registers.cpsr = (word) bs.to_ulong();
}

void arm_7tdmi::execute(arm_instruction instruction) {
    if (!util::condition_met(instruction, *this)) return;

    switch(util::get_instruction_format(instruction)) {

        case DP:
            executeALUInstruction(*this, instruction);
            break;
        default:
            std::cerr << "Cannot execute instruction: " << instruction << "\n";
    }
}