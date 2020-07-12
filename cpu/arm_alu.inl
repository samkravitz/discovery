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
    if (Rn == 15) {
        std::cerr << "Undefined behavior: r15 as operand\n";
        set_state(UND);
        return;
    }
    
    word branch_address = get_register(Rn);
    set_register(15, branch_address); 

    // swith to THUMB mode if necessary
    if ((branch_address & 1) == 1) set_mode(THUMB);
    else set_mode(ARM);
}

inline void arm_7tdmi::branch_link(arm_instruction instruction) {
    bool link = util::get_instruction_subset(instruction, 24, 24) == 0x1;
    word offset = util::get_instruction_subset(instruction, 23, 0);
    bool is_neg = offset >> 23 == 0x1;

    offset <<= 2;

    // maintain sign by padding offset sign extension to 32 bits with 1s
    if (is_neg) {
        offset |= 0b11111100000000000000000000000000;
    }

    if (link) {
        // write the old PC into the link register of the current bank
        // The PC value written into r14 is adjusted to allow for the prefetch, and contains the
        // address of the instruction following the branch and link instruction
        set_register(14, get_register(15) + sizeof(arm_instruction));
    }

    set_register(15, get_register(15) + offset);
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
    uint8_t carry_out = 2;
    
    // determine op2 based on whether it's encoded as an immeidate value or register shift
    if (immediate) {
        op2 = util::get_instruction_subset(instruction, 7, 0);
        uint32_t rotate = util::get_instruction_subset(instruction, 11, 8);
        rotate *= 2; // rotate by twice the value in the rotate field
        // # of bits in a word (should be 32)
        size_t num_bits = sizeof(word) * 8;

        // perform right rotation
        for (int i = 0; i < rotate; ++i) {
            uint8_t dropped_lsb = op2 & 1;  
            op2 >>= 1;
            op2 |= (dropped_lsb << num_bits - 1);
        }
    } else { // op2 is shifted register
        word shifted_register = util::get_instruction_subset(instruction, 3, 0);
        op2 = get_register(shifted_register);
        carry_out = shift_register(instruction, op2);
    }
    
    // for arithmetic operations that use a carry bit, this will either equal
    // - the carry out bit of the barrel shifter (if a register shift was applied), or
    // - the existing condition code flag from the cpsr
    uint8_t carry = carry_out == 2 ? get_condition_code_flag(C) : carry_out;

    // decode opcode (bits 24-21)
    switch((dp_opcodes_t) util::get_instruction_subset(instruction, 24, 21)) {
        case AND: 
            result = op1 & op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_logical(result, carry);
            break;
        case EOR:
            result = op1 ^ op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_logical(result, carry);
            break;
        case SUB:
            result = op1 - op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_subtraction(op1, op2, result);
            break;
        case RSB:
            result = op2 - op1;
            set_register(Rd, result);
            if (set_condition_code) update_flags_subtraction(op2, op1, result);
            break;
        case ADD:
            result = op1 + op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_addition(op1, op2, result);
            break;
        case ADC:
            result = op1 + op2 + carry;
            set_register(Rd, result);
            if (set_condition_code) update_flags_addition(op1, op2, result);
            break;
        case SBC:
            result = op1 - op2 + carry - 1;
            set_register(Rd, result);
            if (set_condition_code) update_flags_subtraction(op1, op2, result);
            break;
        case RSC:
            result = op2 - op1 + carry - 1;
            set_register(Rd, result);
            if (set_condition_code) update_flags_subtraction(op2, op1, result);
            break;
        case TST:
            result = op1 & op2;
            update_flags_logical(result, carry);
            break;
        case TEQ:
            result = op1 ^ op2;
            update_flags_logical(result, carry);
            break;
        case CMP:
            result = op1 - op2;
            update_flags_subtraction(op1, op2, result);
            break;
        case CMN:
            result = op1 + op2;
            update_flags_addition(op1, op2, result);
            break;
        case ORR:
            result = op1 | op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_logical(result, carry);
            break;
        case MOV:
            result = op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_logical(result, carry);
            break;
        case BIC:
            result = op1 & ~op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_logical(result, carry);
            break;
        case MVN:
            result = ~op2;
            set_register(Rd, result);
            if (set_condition_code) update_flags_logical(result, carry);
            break;
        default:
            std::cerr << "Unrecognized data processing opcode: " << util::get_instruction_subset(instruction, 24, 21) << "\n";
            break;
    }

}

inline void arm_7tdmi::multiply(arm_instruction instruction) {
    // assign registers
    word Rm = util::get_instruction_subset(instruction, 3, 0);   // first operand
    word Rs = util::get_instruction_subset(instruction, 11, 8);  // source register
    word Rn = util::get_instruction_subset(instruction, 15, 12); // second operand
    word Rd = util::get_instruction_subset(instruction, 19, 16); // destination register
    bool accumulate = util::get_instruction_subset(instruction, 21, 21);    

    if (Rd == Rm) {
        std::cout << "Rd must not be the same as Rm" << std::endl;
        return;
    } else if (Rd == 15 || Rm == 15) {
        std::cout << "Register 15 may not be used as destination nor operand register" << std::endl;
        return;
    }
    
    word val;
    if (accumulate) {
        // multiply-accumulate form gives Rd:=Rm*Rs+Rn
        val = Rm * Rs + Rn;
    } else {
        // multiply form of the instruction gives Rd:=Rm*Rs,
        // Rn is set to zero for compatibility with possible future instructionset upgrades
        val = Rm * Rs;
        set_register(Rn, 0);
    }
    set_register(Rd, val);
}

// allow access to CPSR and SPSR registers
inline void arm_7tdmi::psr_transfer(arm_instruction instruction) {
    bool spsr = util::get_instruction_subset(instruction, 22, 22) == 1 ? 1 : 0;

    if (util::get_instruction_subset(instruction, 21, 16) == 0b001111) { // MRS (transfer PSR contents to register)
        word Rd = util::get_instruction_subset(instruction, 15, 12);
        if (Rd == 15) {
            std::cerr << "Can't use r15 as a PSR destination register" << "\n";
            return;
        }

        if (spsr) { // Rd <- spsr_<mode>
            set_register(Rd, get_register(17));
        } else { // Rd <- cpsr
            set_register(Rd, get_register(16));
        }
    } else if (util::get_instruction_subset(instruction, 21, 12) == 0b1010011111) { // MSR (transfer register contents to PSR)
        word Rm = util::get_instruction_subset(instruction, 3, 0);
        if (Rm == 15) {
            std::cerr << "Can't use r15 as a PSR source register" << "\n";
            return;
        }
        
        // TODO - is it okay to set entire SPR to contents of Rm?
        // ARM instruction manual says set bits[31:0] but also says to leave reserved bits alone
        if (spsr) { // spsr_<mode> <- Rm
            set_register(17, get_register(Rm));
        } else { // cpsr <- Rm
            set_register(16, get_register(Rm));
        }
    } else if (util::get_instruction_subset(instruction, 21, 12) == 0b1010001111) { // MSR (transfer register contents or immediate value to PSR flag bits only)
        bool immediate = util::get_instruction_subset(instruction, 25, 25) == 1;
        word transfer_value;
        // # of bits in a word (should be 32)
        size_t num_bits = sizeof(word) * 8;

        if (immediate) { // rotate on immediate value 
            transfer_value = util::get_instruction_subset(instruction, 7, 0);
            uint32_t rotate = util::get_instruction_subset(instruction, 11, 8);
            rotate *= 2; // rotate by twice the value in the rotate field

            // perform right rotation
            for (int i = 0; i < rotate; ++i) {
                uint8_t dropped_lsb = transfer_value & 1;  
                transfer_value >>= 1;
                transfer_value |= (dropped_lsb << num_bits - 1);
            }
        } else { // use value in register
            transfer_value = util::get_instruction_subset(instruction, 3, 0);
        }

        // clear bits [27-0] of transfer_value
        transfer_value >>= 28;
        transfer_value <<= 28;

        word old_spr_value;
        if (spsr) {
            old_spr_value = get_register(17);
        } else {
            old_spr_value = get_register(16);
        }

        // move transfer value into flag bits[31:28] of SPR
        old_spr_value <<= 4;
        old_spr_value >>= 4;
        old_spr_value |= transfer_value;
        if (spsr) {
            set_register(17, transfer_value);
        } else {
            set_register(16, transfer_value);
        }

    } else { // should not execute
        std::cerr << "Bad PSR transfer instruction!" << "\n";
        return;
    }
}

// store or load single value to/from memory
inline void arm_7tdmi::single_data_transfer(arm_instruction instruction) {
    bool immediate = util::get_instruction_subset(instruction, 25, 25) == 1;
    bool pre_index = util::get_instruction_subset(instruction, 24, 24) == 1;  // bit 24 set = pre index, bit 24 0 = post index
    bool up = util::get_instruction_subset(instruction, 23, 23) == 1;         // bit 23 set = up, bit 23 0 = down
    bool byte = util::get_instruction_subset(instruction, 22, 22) == 1;       // bit 22 set = byte, bit 23 0 = word
    bool write_back = util::get_instruction_subset(instruction, 21, 21) == 1; // bit 21 set = write address into base, bit 21 0 = no write back
    bool load = util::get_instruction_subset(instruction, 20, 20) == 1;       // bit 20 set = load, bit 20 0 = store
    word Rn = util::get_instruction_subset(instruction, 19, 16);
    word Rd = util::get_instruction_subset(instruction, 15, 12);
    word offset_encoding = util::get_instruction_subset(instruction, 11, 0);
    word offset; // the actual amount to offset

    if (Rd == 15) {
        std::cerr << "r15 may not be used as destination register of SDT." << "\n";
        return;
    }

    if (immediate) {
        offset = offset_encoding;
    } else { // op2 is a shifted register
        word offset_register = util::get_instruction_subset(instruction, 3, 0);
        offset = get_register(offset_register);
        shift_register(instruction, offset); // offset will be modified to contain result of shifted register
    }

    word base = get_register(Rn);

    if (pre_index) { // offset modification before transfer
        if (up) base += offset; // offset is added to base
        else base -= offset; // offset is subtracted from base
    }

    // transfer
    if (load) { // load from memory to register
        if (byte) { // load one byte from memory, sign extend 0s
            word value = 0;
            value |= mem.read_u8(base);
            set_register(Rd, value);
        } else { // load one word from memory
            set_register(Rd, mem.read_u32(base));
        }
    } else { // store from register to memory
        if (byte) { // store one byte to memory
            uint8_t value = get_register(Rd) & 0xFF; // lowest byte in register
            mem.write_u8(base, value);
            
        } else { // store one word into memory

        }
    }
    
}

inline void executeALUInstruction(arm_7tdmi &arm, arm_instruction instruction) {
    std::cout << "Got to the ALU!\n";
}
