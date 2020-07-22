/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: arm_7tdmi.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of arm7tdmi functions
 */
#include <iostream>
#include <bitset>

#include "arm_7tdmi.h"
#include "arm_alu.inl"
#include "util.h"

arm_7tdmi::arm_7tdmi() {
    state = USR;
    mode = ARM;
    registers = {0}; // zero out registers
    registers.r15 = 0x8000000; // starting address of gamepak flash rom
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
        case B:
            branch_link(instruction);
            break;
        case DP:
            data_processing(instruction);
            break;
        case MUL:
            multiply(instruction);
            break;
        case PSR:
            psr_transfer(instruction);
            break;
        case SDT:
            single_data_transfer(instruction);
            break;
        case HDT:
            halfword_data_transfer(instruction);
            break;
        case BDT:
            block_data_transfer(instruction);
            break;
        case SWP:
            single_data_swap(instruction);
            break;
        case INT:
            software_interrupt(instruction);
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

        case 0x8:
            switch (get_state()) {
                case FIQ: return registers.r8_fiq;
                default: return registers.r8;
            }
        case 0x9:
            switch (get_state()) {
                case FIQ: return registers.r9_fiq;
                default: return registers.r9;
            }
        case 0xa:
            switch (get_state()) {
                case FIQ: return registers.r10_fiq;
                default: return registers.r10;
            }
        case 0xb:
            switch (get_state()) {
                case FIQ: return registers.r11_fiq;
                default: return registers.r11;
            }
        case 0xc:
            switch (get_state()) {
                case FIQ: return registers.r12_fiq;
                default: return registers.r12;
            }

        case 0xd:
            switch(get_state()) {
                case USR:
                case SYS: return registers.r13;
                case FIQ: return registers.r13_fiq;
                case SVC: return registers.r13_svc;
                case ABT: return registers.r13_abt;
                case IRQ: return registers.r13_irq;
                case UND: return registers.r13_und;
            }

        case 0xe:
            switch(get_state()) {
                case USR:
                case SYS: return registers.r14;
                case FIQ: return registers.r14_fiq;
                case SVC: return registers.r14_svc;
                case ABT: return registers.r14_abt;
                case IRQ: return registers.r14_irq;
                case UND: return registers.r14_und;
            }

        case 0xf: return registers.r15; // all banks share r15
        case 0x10: return registers.cpsr;// all banks share cpsr
        case 0x11:
            switch(get_state()) {
                case FIQ: return registers.spsr_fiq;
                case SVC: return registers.spsr_svc;
                case ABT: return registers.spsr_abt;
                case IRQ: return registers.spsr_irq;
                case UND: return registers.spsr_und;
            }
            break;
        default:
            std::cerr << "Unknown register: " << reg << "\n";
            return 0;
    }
    return 100; // should never happen
}

void arm_7tdmi::set_register(int reg, word val) {
    switch (reg) {
        // all banks share r0 - r7
        case 0x0: registers.r0 = val; break;
        case 0x1: registers.r1 = val; break;
        case 0x2: registers.r2 = val; break;
        case 0x3: registers.r3 = val; break;
        case 0x4: registers.r4 = val; break;
        case 0x5: registers.r5 = val; break;
        case 0x6: registers.r6 = val; break;
        case 0x7: registers.r7 = val; break;

        case 0x8:
            switch (get_state()) {
                case FIQ:
                    registers.r8_fiq = val;
                    break;
                default:
                    registers.r8 = val;
                    break;
            }
            break;
        case 0x9:
            switch (get_state()) {
                case FIQ:
                    registers.r9_fiq = val;
                    break;
                default:
                    registers.r9 = val;
                    break;
            }
            break;
        case 0xa:
            switch (get_state()) {
                case FIQ:
                    registers.r10_fiq = val;
                    break;
                default:
                    registers.r10 = val;
                    break;
            }
            break;
        case 0xb:
            switch (get_state()) {
                case FIQ:
                    registers.r11_fiq = val;
                    break;
                default:
                    registers.r11 = val;
                    break;
            }
            break;
        case 0xc:
            switch (get_state()) {
                case FIQ:
                    registers.r12_fiq = val;
                    break;
                default:
                    registers.r12 = val;
                    break;
            }
            break;

        case 0xd:
            switch(get_state()) {
                case USR:
                case SYS:
                    registers.r13 = val;
                    break;
                case FIQ:
                    registers.r13_fiq = val;
                    break;
                case SVC:
                    registers.r13_svc = val;
                    break;
                case ABT:
                    registers.r13_abt = val;
                    break;
                case IRQ:
                    registers.r13_irq = val;
                    break;
                case UND:
                    registers.r13_und = val;
                    break;
            }
            break;

        case 0xe:
            switch(get_state()) {
                case USR:
                case SYS:
                    registers.r14 = val;
                    break;
                case FIQ:
                    registers.r14_fiq = val;
                    break;
                case SVC:
                    registers.r14_svc = val;
                    break;
                case ABT:
                    registers.r14_abt = val;
                    break;
                case IRQ:
                    registers.r14_irq = val;
                    break;
                case UND:
                    registers.r14_und = val;
                    break;
            }
            break;


        case 0xf: registers.r15 = val; break; // all banks share r15
        case 0x10: registers.cpsr = val; break; // all banks share cpsr
        case 0x11:
            switch(get_state()) {
                case FIQ:
                    registers.spsr_fiq = val;
                    break;
                case SVC:
                    registers.spsr_svc = val;
                    break;
                case ABT:
                    registers.spsr_abt = val;
                    break;
                case IRQ:
                    registers.spsr_irq = val;
                    break;
                case UND:
                    registers.spsr_und = val;
                    break;
            }
            break;
        default:
            std::cerr << "Unknown register: " << reg << "\n";
            break;
    }
}

// update cpsr flags after a logical operation
void arm_7tdmi::update_flags_logical(word result, uint8_t carry_out) {
    // C flag will be set to the carry out from the barrel shifter
    set_condition_code_flag(C, carry_out);

    // Z flag will be set if and only if the result is all zeros
    uint8_t new_z = result == 0 ? 1 : 0;
    set_condition_code_flag(Z, new_z);

    // N flag will be set to the logical value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    set_condition_code_flag(N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary
}

// update cpsr flags after an addition operation
void arm_7tdmi::update_flags_addition(word op1, word op2, word result) {
    // C flag will be set to the carry out of bit 31 of the ALU
    if (op1 > result || op2 > result) {
        set_condition_code_flag(C, 1);
    } else {
        set_condition_code_flag(C, 0);
    }

    // Z flag will be set if and only if the result was zero
    uint8_t new_z = result == 0 ? 1 : 0;
    set_condition_code_flag(Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    set_condition_code_flag(N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    uint8_t op1_msb = op1 & 0x80000000 ? 1 : 0;
    uint8_t op2_msb = op2 & 0x80000000 ? 1 : 0;
    uint8_t result_msb = result & 0x80000000 ? 1 : 0;
    if (op1_msb == 0 && op2_msb == 0 && result_msb == 1) {
        set_condition_code_flag(V, 1);
    } else if (op1_msb == 1 && op2_msb == 1 && result_msb == 0) {
        set_condition_code_flag(V, 1);
    } else {
        set_condition_code_flag(V, 0);
    }
}

// update cpsr flags after a subtraction operation
void arm_7tdmi::update_flags_subtraction(word op1, word op2, word result) {
    // C flag will be set to the carry out of bit 31 of the ALU
    if (result > op1 || result > op2) {
        set_condition_code_flag(C, 1);
    } else {
        set_condition_code_flag(C, 0);
    }

    // Z flag will be set if and only if the result was zero
    uint8_t new_z = result == 0 ? 1 : 0;
    set_condition_code_flag(Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    set_condition_code_flag(N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    uint8_t op1_msb = op1 & 0x80000000 ? 1 : 0;
    uint8_t op2_msb = op2 & 0x80000000 ? 1 : 0;
    uint8_t result_msb = result & 0x80000000 ? 1 : 0;
    if (op1_msb == 0 && op2_msb == 1 && result_msb == 1) {
        set_condition_code_flag(V, 1);
    } else if (op1_msb == 1 && op2_msb == 0 && result_msb == 0) {
        set_condition_code_flag(V, 1);
    } else {
        set_condition_code_flag(V, 0);
    }
}

// performs a shifted register operation on op2.
// returns the carry-out of the barrel shifter
uint8_t arm_7tdmi::shift_register(arm_instruction instruction, word &op2) {
    word shift = util::get_instruction_subset(instruction, 11, 4);
    word shift_type = util::get_instruction_subset(instruction, 6, 5);
    word shift_amount;
    uint8_t carry_out;
    // # of bits in a word (should be 32)
    size_t num_bits = sizeof(word) * 8;

    // get shift amount
    if ((shift & 1) == 1) { // shift amount contained in bottom byte of Rs
        word Rs = util::get_instruction_subset(instruction, 11, 8);
        shift_amount = get_register(Rs) & 0xFF;
        // if this amount is 0, skip shifting
        if (shift_amount == 0) return 2;
    } else { // shift contained in immediate value in instruction
        shift_amount = util::get_instruction_subset(instruction, 11, 7);
    }

    // perform shift
    switch (shift_type) {
        // LSL
        case 0b00:
            if (shift_amount == 0) carry_out = get_condition_code_flag(C); // preserve C flag
            for (int i = 0; i < shift_amount; ++i) {
                carry_out = (op2 >> num_bits - 1) & 1;
                op2 <<= 1;
            }
            break;
        
        // LSR
        case 0b01:
            if (shift_amount != 0) { // normal LSR
                for (int i = 0; i < shift_amount; ++i) {
                    carry_out = op2 & 1;
                    op2 >>= 1;
                }
            } else { // special encoding for LSR #32
                carry_out = (op2 >> num_bits - 1) & 1;
                op2 = 0;
            }
            break;
        
        // ASR
        case 0b10:
            if (shift_amount != 0) {
                for (int i = 0; i < shift_amount; ++i) {
                    carry_out  = op2 & 1;
                    uint8_t msb = (op2 >> num_bits - 1) & 1; // most significant bit
                    op2 >>= 1;
                    op2 = op2 | (msb << num_bits - 1);
                }
            } else { // special encoding for ASR #32
                carry_out = (op2 >> num_bits - 1) & 1;
                op2 = carry_out == 0 ? 0 : (word) ~0;
            }
            break;
        
        // ROR
        case 0b11:
            if (shift_amount != 0) { // normal rotate right
                for (int i = 0; i < shift_amount; ++i) {
                    carry_out = op2 & 1;
                    uint8_t dropped_lsb = op2 & 1;  
                    op2 >>= 1;
                    op2 |= (dropped_lsb << num_bits - 1);
                }
            } else { // rotate right extended
                carry_out = op2 & 1;
                op2 >>= 1;
                op2 |= (get_condition_code_flag(C) << num_bits - 1);
            }
            break;
    }
    return carry_out;
}