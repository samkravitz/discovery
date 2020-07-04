/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: arm_alu.inl
 * DATE: June 27, 2020
 * DESCRIPTION: execution of arm instructions
 */

#include <iostream>
#include "util.h"
#include "arm_7tdmi.h"

/*
 * Copies the contents of Rn (bits 3-0) of instruction into the PC,
 * flushes the pipeline, and restarts execution from the address
 * contained in Rn. If bit 0 of Rn is 1, switch to THUMB mode
 */
inline void arm_7tdmi::branch_exchange(arm_instruction instruction) {
    uint32_t Rn = util::get_instruction_subset(instruction, 3, 0);
    switch (Rn) {
        case 0x0:
            registers.r15 = registers.r0;
            break;
        case 0x1:
            registers.r15 = registers.r1;
            break;
        case 0x2:
            registers.r15 = registers.r2;
            break;
        case 0x3:
            registers.r15 = registers.r3;
            break;
        case 0x4:
            registers.r15 = registers.r4;
            break;
        case 0x5:
            registers.r15 = registers.r5;
            break;
        case 0x6:
            registers.r15 = registers.r6;
            break;
        case 0x7:
            registers.r15 = registers.r7;
            break;
        case 0x8:
            registers.r15 = registers.r8;
            break;
        case 0x9:
            registers.r15 = registers.r9;
            break;
        case 0xA:
            registers.r15 = registers.r10;
            break;
        case 0xB:
            registers.r15 = registers.r11;
            break;
        case 0xC:
            registers.r15 = registers.r12;
            break;
        case 0xD:
            registers.r15 = registers.r13;
            break;
        case 0xE:
            registers.r15 = registers.r14;
            break;
        case 0xF:
            std::cerr << "Undefined behavior: r15 as operand\n";
            set_state(UND);
            return;
        default:
            std::cerr << "Unknown register: " << Rn << "\n";
            break;
    }   

    // swith to THUMB mode if necessary
    if (Rn % 2 == 1) set_mode(THUMB);
    else set_mode(ARM);
}

inline void arm_7tdmi::data_processing(arm_instruction instruction) {
    // immediate operand bit
    bool immediate = util::get_instruction_subset(instruction, 25, 25) == 0x1;
    // set condition code
    bool set_condition_code = util::get_instruction_subset(instruction, 20, 20) == 0x1;

    word Rd = util::get_instruction_subset(instruction, 15, 12); // destination register
    word Rn = util::get_instruction_subset(instruction, 19, 16); // source register
    word op1 = get_register(Rn);
    word op2;
    word result;

    // # of bits in a word (should be 32)
    size_t num_bits = sizeof(word) * 8;
    
    // determine op2 based on whether it's encoded as an immeidate value or register shift
    if (immediate) {
        op2 = util::get_instruction_subset(instruction, 7, 0);
        uint32_t rotate = util::get_instruction_subset(instruction, 11, 8);
        rotate *= 2; // rotate by twice the value in the rotate field

        // perform right rotation
        for (int i = 0; i < rotate; ++i) {
            uint8_t dropped_lsb = op2 & 1;  
            op2 >>= 1;
            op2 = op2 | (dropped_lsb << num_bits - 1);
        }
    } else { // op2 is shifted register
        op2 = op1;
        word shift = util::get_instruction_subset(instruction, 11, 4);
        word shifted_register = util::get_instruction_subset(instruction, 3, 0);
        word shift_type = util::get_instruction_subset(instruction, 6, 5);
        word shift_amount;
        uint8_t carry_out = get_condition_code_flag(C);

        // get shift amount
        if ((shift & 1) == 1) { // shift amount contained in bottom byte of Rs
            word Rs = util::get_instruction_subset(instruction, 11, 8);
            shift_amount = get_register(Rs) & 0xFF;
            // if this amount is 0, skip shifting
            if (shift_amount == 0) goto decode_opcode;
        } else { // shift contained in immediate value in instruction
            shift_amount = util::get_instruction_subset(instruction, 11, 7);
        }

        // perform shift
        switch (shift_type) {
            // LSL
            case 0b00:
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
                        uint8_t msb = (op2 >> num_bits - 1) & 1;; // most significant bit
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
                        op2 = op2 | (dropped_lsb << num_bits - 1);
                    }
                } else { // rotate right extended
                    carry_out = op2 & 1;
                    op2 >>= 1;
                    op2 = op2 | (get_condition_code_flag(C) << num_bits - 1);
                }
                break;
        }
        set_condition_code_flag(C, carry_out);
    }

    decode_opcode:
    // decode opcode (bits 24-21)
    switch((dp_opcodes_t) util::get_instruction_subset(instruction, 24, 21)) {
        case AND: 
            result = op1 & op2;
            set_register(Rd, result);
            break;
        case EOR:
            result = op1 ^ op2;
            set_register(Rd, result);
            break;
        case SUB:
            result = op1 - op2;
            set_register(Rd, result);
            break;
        case RSB:
            result = op2 - op1;
            set_register(Rd, result);
            break;
        case ADD:
            result = op1 + op2;
            set_register(Rd, result);
            break;
        case ADC:
            result = op1 + op2 + get_condition_code_flag(C);
            set_register(Rd, result);
            break;
        case SBC:
            result = op1 - op2 + get_condition_code_flag(C) - 1;
            set_register(Rd, result);
            break;
        case RSC:
            result = op2 - op1 + get_condition_code_flag(C) - 1;
            set_register(Rd, result);
            break;
        default:
            std::cerr << "Unrecognized data processing opcode: " << util::get_instruction_subset(instruction, 24, 21) << "\n";
            break;
    }

}

inline void executeALUInstruction(arm_7tdmi &arm, arm_instruction instruction) {
    std::cout << "Got to the ALU!\n";
}