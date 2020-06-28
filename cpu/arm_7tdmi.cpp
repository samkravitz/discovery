#include <iostream>
#include <bitset>

#include "arm_7tdmi.h"
#include "arm_alu.inl"
#include "util.h"

arm_7tdmi::arm_7tdmi() {
    state = USR;
    mode = ARM;
    registers = {}; // zero out registers
}

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
        case BEX:
            branch_exchange(instruction);
            break;
        case DP:
            data_processing(instruction);
            break;
        default:
            std::cerr << "Cannot execute instruction: " << instruction << "\n";
    }
}

word arm_7tdmi::get_register(uint32_t reg) {
    switch (reg) {
        case 0x0: return registers.r0;
        case 0x1: return registers.r1;
        case 0x2: return registers.r2;
        case 0x3: return registers.r3;
        case 0x4: return registers.r4;
        case 0x5: return registers.r5;
        case 0x6: return registers.r6;
        case 0x7: return registers.r7;
        case 0x8: return registers.r8;
        case 0x9: return registers.r9;
        case 0xa: return registers.r10;
        case 0xb: return registers.r11;
        case 0xc: return registers.r12;
        case 0xd: return registers.r13;
        case 0xe: return registers.r14;
        case 0xf: return registers.r15;
        case 0x10: return registers.cpsr;
        default:
            std::cerr << "Unknown register: " << reg << "\n";
            return 0;
    }
}

void arm_7tdmi::set_register(uint32_t reg, word val) {
    switch (reg) {
        case 0x0: registers.r0 = val; break;
        case 0x1: registers.r1 = val; break;
        case 0x2: registers.r2 = val; break;
        case 0x3: registers.r3 = val; break;
        case 0x4: registers.r4 = val; break;
        case 0x5: registers.r5 = val; break;
        case 0x6: registers.r6 = val; break;
        case 0x7: registers.r7 = val; break;
        case 0x8: registers.r8 = val; break;
        case 0x9: registers.r9 = val; break;
        case 0xa: registers.r10 = val; break;
        case 0xb: registers.r11 = val; break;
        case 0xc: registers.r12 = val; break;
        case 0xd: registers.r13 = val; break;
        case 0xe: registers.r14 = val; break;
        case 0xf: registers.r15 = val; break;
        case 0x10: registers.cpsr = val; break;
        default:
            std::cerr << "Unknown register: " << reg << "\n";
            break;
    }
}