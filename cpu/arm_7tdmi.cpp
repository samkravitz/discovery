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

#include "arm_7tdmi.h"
#include "arm_alu.inl"
#include "util.h"

// uncomment this if running tests
#define TEST

arm_7tdmi::arm_7tdmi() {
    state = SYS;
    mode = ARM;
    registers = {0}; // zero out registers
    registers.cpsr.bits.state = SYS;

    // initialize cpsr
    registers.cpsr.bits.i = 1;
    registers.cpsr.bits.f = 1;

    registers.r15 = 0x8000000; // starting address of gamepak flash rom
    mem = NULL;

    // different initialization for the testing environment
    #ifdef TEST
    registers.r15 = 0;
    state = USR;
    mem = new Memory();
    #endif
}

uint8_t arm_7tdmi::get_condition_code_flag(condition_code_flag_t flag) {
    switch (flag) {
        case N: return registers.cpsr.bits.n; 
        case Z: return registers.cpsr.bits.z;
        case C: return registers.cpsr.bits.c;
        case V: return registers.cpsr.bits.v;
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

    switch (flag) {
        case N: registers.cpsr.bits.n = bit; break;
        case Z: registers.cpsr.bits.z = bit; break;
        case C: registers.cpsr.bits.c = bit; break;
        case V: registers.cpsr.bits.v = bit; break;
        default:
            std::cerr << "Unrecognized condition code flag: " << flag <<"\n";
            return;
    }
}

 // determine if the condition field of an instruction is true, given the state of the CPSR
bool arm_7tdmi::condition_met(condition_t condition_field) {
    switch (condition_field) {
        case EQ: return get_condition_code_flag(Z); // Z set
        case NE: return !get_condition_code_flag(Z); // Z clear
        case CS: return get_condition_code_flag(C); // C set
        case CC: return !get_condition_code_flag(C); // C clear
        case MI: return get_condition_code_flag(N); // N set
        case PL: return !get_condition_code_flag(N); // N Clear
        case VS: return get_condition_code_flag(V); // V set
        case VC: return !get_condition_code_flag(V); // V clear
        case HI: return get_condition_code_flag(C) && !get_condition_code_flag(Z); // C set and Z clear
        case LS: return !get_condition_code_flag(C) || get_condition_code_flag(Z); // C clear or Z set
        case GE: return get_condition_code_flag(N) == get_condition_code_flag(V); // N equals V
        case LT: return get_condition_code_flag(N) != get_condition_code_flag(V); // N not equal V
        case GT: return !get_condition_code_flag(Z) && (get_condition_code_flag(N) == get_condition_code_flag(V)); // Z clear AND (N equals V)
        case LE: return get_condition_code_flag(Z) || (get_condition_code_flag(N) != get_condition_code_flag(V)); // Z set OR (N not equal to V)
        case AL: return true; // always
        default: // should never happen
            std::cerr << "Unrecognized condition field: " << condition_field << "\n";
            return false;
    }
}

void arm_7tdmi::fetch() {
    switch (mode) {
        case ARM:
            current_instruction = mem->read_u32(registers.r15);
            break;
        case THUMB:
            current_instruction = (u16) mem->read_u16(registers.r15);
            break;
    }
}

void arm_7tdmi::execute(u32 instruction) {
    switch (mode) {
        case ARM:
            if (!condition_met((condition_t) util::get_instruction_subset(instruction, 31, 28))) {
                increment_pc();
                return;
            }
            
            switch(util::get_instruction_format(instruction)) {
                case BEX:
                    branch_exchange(instruction);
                    break;
                case B:
                    branch_link(instruction);
                    break;
                case DP:
                    data_processing(instruction);
                    increment_pc();
                    break;
                case MUL:
                    multiply(instruction);
                    increment_pc();
                    break;
                case PSR:
                    psr_transfer(instruction);
                    increment_pc();
                    break;
                case SDT:
                    single_data_transfer(instruction);
                    increment_pc();
                    break;
                case HDT:
                    halfword_data_transfer(instruction);
                    increment_pc();
                    break;
                case BDT:
                    block_data_transfer(instruction);
                    increment_pc();
                    break;
                case SWP:
                    single_data_swap(instruction);
                    increment_pc();
                    break;
                case INT:
                    software_interrupt(instruction);
                    increment_pc();
                    break;
                default:
                    std::cerr << "Cannot execute instruction: " << instruction << "\n";
            }
            break;

        case THUMB:
            std::cout << "Instruction thumb type is: " << util::get_instruction_format((u16) instruction) << "\n";
            switch(util::get_instruction_format((u16) instruction)) {
                case MSR_T:
                    move_shifted_register_thumb((u16) instruction);
                    increment_pc();
                    break;
                case ADDSUB_T:
                    add_sub_thumb((u16) instruction);
                    increment_pc();
                    break;
                case IMM_T:
                    move_immediate_thumb((u16) instruction);
                    increment_pc();
                    break;
                case ALU_T:
                    alu_thumb((u16) instruction);
                    increment_pc();
                    break;
                case HI_T:
                    hi_reg_ops_thumb((u16) instruction);
                    increment_pc();
                    break;
                case PC_T:
                    pc_rel_load_thumb((u16) instruction);
                    increment_pc();
                    break;
                case MOV_T:
                    load_store_reg((u16) instruction);
                    increment_pc();
                    break;
                case MOVS_T:
                    load_store_signed_halfword((u16) instruction);
                    increment_pc();
                    break;
                case MOVI_T:
                    load_store_immediate((u16) instruction);
                    increment_pc();
                    break;
                case MOVH_T:
                    load_store_halfword((u16) instruction);
                    increment_pc();
                    break;
                case SP_T:
                    sp_load_store((u16) instruction);
                    increment_pc();
                    break;
                case LDA_T:
                    load_address((u16) instruction);
                    increment_pc();
                    break;
                case ADDSP_T:
                    add_offset_to_sp((u16) instruction);
                    increment_pc();
                    break;
                case POP_T:
                    push_pop((u16) instruction);
                    increment_pc();
                    break;
                case MOVM_T:
                    multiple_load_store((u16) instruction);
                    increment_pc();
                    break;
                case B_T:
                    conditional_branch((u16) instruction);
                    break;
                default:
                    std::cerr << "Cannot execute thumb instruction: " << (u16) instruction << "\n";
                    break;
            }
        break;
    }
}

u32 arm_7tdmi::get_register(uint32_t reg) {
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
        case 0x10: return registers.cpsr.full;// all banks share cpsr
        case 0x11:
            switch(get_state()) {
                case FIQ: return registers.spsr_fiq.full;
                case SVC: return registers.spsr_svc.full;
                case ABT: return registers.spsr_abt.full;
                case IRQ: return registers.spsr_irq.full;
                case UND: return registers.spsr_und.full;
            }
            break;
        default:
            std::cerr << "Unknown register: " << reg << "\n";
            return 0;
    }
    return 100; // should never happen
}

void arm_7tdmi::set_register(int reg, u32 val) {
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
        case 0x10: update_psr(false, val); break; // all banks share cpsr
        case 0x11: update_psr(true, val); break; // special case for spsr
        default:
            std::cerr << "Unknown register: " << reg << "\n";
            break;
    }
}

// update cpsr flags after a logical operation
void arm_7tdmi::update_flags_logical(u32 result, uint8_t carry_out) {
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
void arm_7tdmi::update_flags_addition(u32 op1, u32 op2, u32 result) {
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
void arm_7tdmi::update_flags_subtraction(u32 op1, u32 op2, u32 result) {
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

/* performs a shifted register operation on op2.
 * returns the carry-out of the barrel shifter
 *
 *  @params:
 *  shift_amount - the amount of times to shift
 *  num - the number that will actually be shifted
 *  opcode - an encoding of which type of shift to be performed
 */
uint8_t arm_7tdmi::shift_register(u32 shift_amount, u32 &num, u8 opcode) {
    uint8_t carry_out;
    // # of bits in a word (should be 32)
    size_t num_bits = sizeof(u32) * 8;

    // perform shift
    switch (opcode) {
        // LSL
        case 0b00:
            if (shift_amount == 0) carry_out = get_condition_code_flag(C); // preserve C flag
            for (int i = 0; i < shift_amount; ++i) {
                carry_out = (num >> num_bits - 1) & 1;
                num <<= 1;
            }
            break;
        
        // LSR
        case 0b01:
            if (shift_amount != 0) { // normal LSR
                for (int i = 0; i < shift_amount; ++i) {
                    carry_out = num & 1;
                    num >>= 1;
                }
            } else { // special encoding for LSR #32
                carry_out = (num >> num_bits - 1) & 1;
                num = 0;
            }
            break;
        
        // ASR
        case 0b10:
            if (shift_amount != 0) {
                for (int i = 0; i < shift_amount; ++i) {
                    carry_out  = num & 1;
                    uint8_t msb = (num >> num_bits - 1) & 1; // most significant bit
                    num >>= 1;
                    num |= (msb << num_bits - 1);
                }
            } else { // special encoding for ASR #32
                carry_out = (num >> num_bits - 1) & 1;
                num = carry_out == 0 ? 0 : (u32) ~0;
            }
            break;
        
        // ROR
        case 0b11:
            if (shift_amount != 0) { // normal rotate right
                for (int i = 0; i < shift_amount; ++i) {
                    carry_out = num & 1;
                    uint8_t dropped_lsb = num & 1;  
                    num >>= 1;
                    num |= (dropped_lsb << num_bits - 1);
                }
            } else { // rotate right extended
                carry_out = num & 1;
                num >>= 1;
                num |= (get_condition_code_flag(C) << num_bits - 1);
            }
            break;
    }

    return carry_out;
}

inline void arm_7tdmi::increment_pc() {
    u32 instruction_ptr = get_register(15);
    instruction_ptr += get_mode() == ARM ? 4 : 2;
    set_register(15, instruction_ptr);
}


/*
 * Updates the value in the psr
 * Can also change the emulator's state or mode depending on the value
 * parameter spsr is true if the psr in question is current bank's spsr
 * else it is the cpsr
 */ 
void arm_7tdmi::update_psr(bool spsr, u32 value) {
    if (spsr) {
        switch (state) {
            case USR:
                std::cerr << "Error: SPSR does not exist in user mode" << "\n";
                return;
            case FIQ:
                registers.spsr_fiq.full = value;
                break;
            case SVC:
                registers.spsr_svc.full = value;
                break;
            case ABT:
                registers.spsr_abt.full = value;
                break;
            case IRQ:
                registers.spsr_irq.full = value;
                break;
            case UND:
                registers.spsr_und.full = value;
                break;
        }

        return;
    }

    // cpsr
    status_register sr;
    sr.full = value;
    switch (state) {
        case USR: // in user mode, only condition bits can be changed
            registers.cpsr.bits.n = sr.bits.n;
            registers.cpsr.bits.z = sr.bits.z;
            registers.cpsr.bits.c = sr.bits.c;
            registers.cpsr.bits.v = sr.bits.v;
            return;
        case FIQ:
        case SVC:
        case ABT:
        case IRQ:
        case UND:
            switch (sr.bits.state) {
                case USR:
                    registers.cpsr.bits.state = sr.bits.state;
                    set_state(USR);
                    break;
                case FIQ:
                    if (registers.cpsr.bits.f == 1) break; // fiq disabled bit set
                    registers.cpsr.bits.state = sr.bits.state;
                    set_state(FIQ);
                    break;
                case SVC:
                    registers.cpsr.bits.state = sr.bits.state;
                    set_state(SVC);
                    break;
                case ABT:
                    registers.cpsr.bits.state = sr.bits.state;
                    set_state(ABT);
                    break;
                case IRQ:
                    if (registers.cpsr.bits.i == 1) break; // irq disabled bit set
                    registers.cpsr.bits.state = sr.bits.state;
                    set_state(IRQ);
                    break;
                case UND:
                    registers.cpsr.bits.state = sr.bits.state;
                    set_state(UND);
                    break;
            }
        break;
    }

    // update N, Z, C, V, I, F, and T bits of cpsr
    registers.cpsr.bits.n = sr.bits.n;
    registers.cpsr.bits.z = sr.bits.z;
    registers.cpsr.bits.c = sr.bits.c;
    registers.cpsr.bits.v = sr.bits.v;

    registers.cpsr.bits.i = sr.bits.i; // irq disable flag
    registers.cpsr.bits.f = sr.bits.f; // fiq disable flag

    if (registers.cpsr.bits.t != sr.bits.n) {
        std::cout << "Software is changing TBIT in CPSR!" << "\n"; // is this allowed??
    }

    registers.cpsr.bits.t = sr.bits.t;
    if (sr.bits.t == 1) set_mode(THUMB);
    else set_mode(ARM);
}