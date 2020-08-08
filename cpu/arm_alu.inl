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
inline void arm_7tdmi::branch_exchange(u32 instruction) {
    uint32_t Rn = util::get_instruction_subset(instruction, 3, 0);
    if (Rn == 15) {
        std::cerr << "Undefined behavior: r15 as operand\n";
        set_state(UND);
        return;
    }
    
    u32 branch_address = get_register(Rn);
    set_register(15, branch_address); 

    // swith to THUMB mode if necessary
    if ((branch_address & 1) == 1) {
        registers.r15 -= 1; // continue at Rn - 1 for thumb mode
        set_mode(THUMB);
        registers.cpsr.bits.t = 1; // TBIT
    }

    // flush pipeline for refill
    pipeline_full = false;
}

inline void arm_7tdmi::branch_link(u32 instruction) {
    bool link = util::get_instruction_subset(instruction, 24, 24) == 0x1;
    u32 offset = util::get_instruction_subset(instruction, 23, 0);
    bool is_neg = offset >> 23 == 0x1;
    u32 new_address;

    offset <<= 2;

    // maintain sign by padding offset sign extension to 32 bits with 1s
    if (is_neg) {
        offset |= 0b11111100000000000000000000000000;
    }

    if (link) {
        // write the old PC into the link register of the current bank
        // The PC value written into r14 is adjusted to allow for the prefetch, and contains the
        // address of the instruction following the branch and link instruction
        new_address = pipeline[1];
        new_address &= ~3; // clear bits 0-1
        set_register(14, new_address);
    }

    new_address = get_register(15) + offset;
    set_register(15, new_address);

    // flush pipeline for refill
    pipeline_full = false;
}

inline void arm_7tdmi::data_processing(u32 instruction) {
    // immediate operand bit
    bool immediate = util::get_instruction_subset(instruction, 25, 25) == 0x1;
    // set condition code
    bool set_condition_code = util::get_instruction_subset(instruction, 20, 20) == 0x1;

    u32 Rd = util::get_instruction_subset(instruction, 15, 12); // destination register
    u32 Rn = util::get_instruction_subset(instruction, 19, 16); // source register
    u32 op1 = get_register(Rn);

    // if (Rn == 15) {
    //     if (immediate) op1 += 8;
    //     else op1 += 12;
    // }
    
    u32 op2;
    u32 result;
    uint8_t carry_out = 2;
    
    // determine op2 based on whether it's encoded as an immeidate value or register shift
    if (immediate) {
        op2 = util::get_instruction_subset(instruction, 7, 0);
        uint32_t rotate = util::get_instruction_subset(instruction, 11, 8);
        rotate *= 2; // rotate by twice the value in the rotate field
        // # of bits in a word (should be 32)
        size_t num_bits = sizeof(u32) * 8;

        // perform right rotation
        for (int i = 0; i < rotate; ++i) {
            uint8_t dropped_lsb = op2 & 1;  
            op2 >>= 1;
            op2 |= (dropped_lsb << num_bits - 1);
        }
    } else { // op2 is shifted register
        u32 shifted_register = util::get_instruction_subset(instruction, 3, 0);
        u32 shift = util::get_instruction_subset(instruction, 11, 4);
        u32 shift_amount;
        u8 shift_type = util::get_instruction_subset(instruction, 6, 5);

        // get shift amount
        if ((shift & 1) == 1) { // shift amount contained in bottom byte of Rs
            u32 Rs = util::get_instruction_subset(instruction, 11, 8);
            shift_amount = get_register(Rs) & 0xFF;
            // if this amount is 0, skip shifting
            //if (shift_amount == 0) return 2;
        } else { // shift contained in immediate value in instruction
            shift_amount = util::get_instruction_subset(instruction, 11, 7);
        }

        op2 = get_register(shifted_register);
        carry_out = shift_register(shift_amount, op2, shift_type);
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

inline void arm_7tdmi::multiply(u32 instruction) {
    // assign registers
    u32 Rm = util::get_instruction_subset(instruction, 3, 0);   // first operand
    u32 Rs = util::get_instruction_subset(instruction, 11, 8);  // source register
    u32 Rn = util::get_instruction_subset(instruction, 15, 12); // second operand
    u32 Rd = util::get_instruction_subset(instruction, 19, 16); // destination register
    bool accumulate = util::get_instruction_subset(instruction, 21, 21);    

    if (Rd == Rm) {
        std::cout << "Rd must not be the same as Rm" << std::endl;
        return;
    } else if (Rd == 15 || Rm == 15) {
        std::cout << "Register 15 may not be used as destination nor operand register" << std::endl;
        return;
    }
    
    u32 val;
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
inline void arm_7tdmi::psr_transfer(u32 instruction) {
    bool spsr = util::get_instruction_subset(instruction, 22, 22) == 1 ? 1 : 0;

    if (util::get_instruction_subset(instruction, 21, 16) == 0b001111) { // MRS (transfer PSR contents to register)
        u32 Rd = util::get_instruction_subset(instruction, 15, 12);
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
        u32 Rm = util::get_instruction_subset(instruction, 3, 0);
        if (Rm == 15) {
            std::cerr << "Can't use r15 as a PSR source register" << "\n";
            return;
        }
        
        update_psr(spsr, get_register(Rm));
    } else if (util::get_instruction_subset(instruction, 21, 12) == 0b1010001111) { // MSR (transfer register contents or immediate value to PSR flag bits only)
        bool immediate = util::get_instruction_subset(instruction, 25, 25) == 1;
        u32 transfer_value;
        // # of bits in a word (should be 32)
        size_t num_bits = sizeof(u32) * 8;

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
            u32 Rm = util::get_instruction_subset(instruction, 3, 0);
            transfer_value = get_register(Rm);
        }

        // clear bits [27-0] of transfer_value
        transfer_value >>= 28;
        transfer_value <<= 28;

        u32 old_spr_value;
        if (spsr) {
            old_spr_value = get_register(17);
        } else {
            old_spr_value = get_register(16);
        }

        // move transfer value into flag bits[31:28] of SPR
        old_spr_value <<= 4;
        old_spr_value >>= 4;
        old_spr_value |= transfer_value;
        update_psr(spsr, transfer_value);

    } else { // should not execute
        std::cerr << "Bad PSR transfer instruction!" << "\n";
        return;
    }
}

// store or load single value to/from memory
inline void arm_7tdmi::single_data_transfer(u32 instruction) {
    bool immediate = util::get_instruction_subset(instruction, 25, 25) == 0;
    bool pre_index = util::get_instruction_subset(instruction, 24, 24) == 1;  // bit 24 set = pre index, bit 24 0 = post index
    bool up = util::get_instruction_subset(instruction, 23, 23) == 1;         // bit 23 set = up, bit 23 0 = down
    bool byte = util::get_instruction_subset(instruction, 22, 22) == 1;       // bit 22 set = byte, bit 23 0 = word
    bool write_back = util::get_instruction_subset(instruction, 21, 21) == 1; // bit 21 set = write address into base, bit 21 0 = no write back
    bool load = util::get_instruction_subset(instruction, 20, 20) == 1;       // bit 20 set = load, bit 20 0 = store
    u32 Rn = util::get_instruction_subset(instruction, 19, 16);
    u32 Rd = util::get_instruction_subset(instruction, 15, 12);
    u32 offset_encoding = util::get_instruction_subset(instruction, 11, 0);
    u32 offset; // the actual amount to offset
    
    if (Rd == 15) {
        std::cerr << "r15 may not be used as destination register of SDT." << "\n";
        return;
    }

    if (immediate) {
        offset = offset_encoding;
    } else { // op2 is a shifted register
        u32 shift_amount = util::get_instruction_subset(instruction, 11, 7);
        u32 offset_register = util::get_instruction_subset(instruction, 3, 0);
        offset = get_register(offset_register);
        u8 shift_type = util::get_instruction_subset(instruction, 6, 5);
        shift_register(shift_amount, offset, shift_type); // offset will be modified to contain result of shifted register
    }

    u32 base = get_register(Rn);

    if (pre_index) { // offset modification before transfer
        if (up) base += offset; // offset is added to base
        else base -= offset; // offset is subtracted from base
    }

    // transfer
    if (load) { // load from memory to register
        if (byte) { // load one byte from memory, sign extend 0s
            u32 value = 0;
            value |= mem->read_u8(base);
            set_register(Rd, value);
        } else { // load one word from memory
            set_register(Rd, mem->read_u32(base));
        }
    } else { // store from register to memory
        if (byte) { // store one byte to memory
            uint8_t value = get_register(Rd) & 0xFF; // lowest byte in register
            mem->write_u8(base, value);
        } else { // store one word into memory
            mem->write_u32(base, get_register(Rd));
        }
    }

    if (!pre_index) { // offset modification after transfer
        if (up) base += offset; // offset is added to base
        else base -= offset; // offset is subtracted from base
    }

    if ((write_back || !pre_index) && Rn != Rd && Rn != 15) {
        set_register(Rn, base);
    } 
    
}

// transfer halfword and signed data
inline void arm_7tdmi::halfword_data_transfer(u32 instruction) {
    bool pre_index = util::get_instruction_subset(instruction, 24, 24) == 1;  // bit 24 set = pre index, bit 24 0 = post index
    bool up = util::get_instruction_subset(instruction, 23, 23) == 1;         // bit 23 set = up, bit 23 0 = down
    bool immediate = util::get_instruction_subset(instruction, 22, 22) == 1;
    bool write_back = util::get_instruction_subset(instruction, 21, 21) == 1; // bit 21 set = write address into base, bit 21 0 = no write back
    bool load = util::get_instruction_subset(instruction, 20, 20) == 1;       // bit 20 set = load, bit 20 0 = store
    u32 Rn = util::get_instruction_subset(instruction, 19, 16);              // base register
    u32 Rd = util::get_instruction_subset(instruction, 15, 12);              // src/dest register
    u32 Rm = util::get_instruction_subset(instruction, 3, 0);                // offset register
    u32 offset;
    u32 base = get_register(Rn);

    if (Rm == 15) {
        std::cerr << "r15 cannot be used as offset register for HDT" << "\n";
    }

    if (immediate) {
        u32 high_nibble = util::get_instruction_subset(instruction, 11, 8);
        u32 low_nibble = util::get_instruction_subset(instruction, 3, 0);
        offset = (high_nibble << 4) | low_nibble;
    } else {
        offset = get_register(Rm);
    }

    if (pre_index) { // offset modification before transfer
        if (up) base += offset; // offset is added to base
        else base -= offset; // offset is subtracted from base
    }

    // transfer
    switch (util::get_instruction_subset(instruction, 6, 5)) { // SH bits
        case 0b01: // unsigned halfwords
                if (load) {
                    set_register(Rd, mem->read_u16(base));
                } else {
                    mem->write_u16(base, get_register(Rd) & 0xFFFF);
                }
            break;
        
        case 0b10: // signed byte
                if (load) {
                    u32 value = (u32) mem->read_u8(base);
                    if (value & 0x80) value |= ~0b11111111; // bit 7 of byte is 1, so sign extend bits 31-8 of register
                    set_register(Rd, value);
                } else {
                    std::cerr << "Cannot store a signed byte in HDT!" << "\n";
                    return;
                }
            break;
        
        case 0b11: // signed halfwords
                if (load) {
                    u32 value = (u32) mem->read_u16(base);
                    if (value & 0x8000) value |= ~0b1111111111111111; // bit 15 of byte is 1, so sign extend bits 31-8 of register
                    set_register(Rd, value);
                } else {
                    std::cerr << "Cannot store a signed byte in HDT!" << "\n";
                    return;
                }
            break;
        
        default:
            std::cerr << "SH bits are 00! SWP instruction was decoded as HDT!" << "\n";
            return;
    }

    if (!pre_index) {
        if (up) base += offset; // offset is added to base
        else base -= offset; // offset is subtracted from base
    }

    if ((write_back || !pre_index) && Rn != 15) {
        set_register(Rn, base);
    }
}

inline void arm_7tdmi::block_data_transfer(u32 instruction) {
    bool pre_index = util::get_instruction_subset(instruction, 24, 24) == 1;  // bit 24 set = pre index, bit 24 0 = post index
    bool up = util::get_instruction_subset(instruction, 23, 23) == 1;         // bit 23 set = up, bit 23 0 = down
    bool load_psr = util::get_instruction_subset(instruction, 22, 22) == 1;   // bit 22 set = load PSR or force user mode
    bool write_back = util::get_instruction_subset(instruction, 21, 21) == 1; // bit 21 set = write address into base, bit 21 0 = no write back
    bool load = util::get_instruction_subset(instruction, 20, 20) == 1;       // bit 20 set = load, bit 20 0 = store
    u32 Rn = util::get_instruction_subset(instruction, 19, 16);              // base register
    u32 register_list = util::get_instruction_subset(instruction, 15, 0);
    u32 base = get_register(Rn);
    int num_registers = 0; // number of set bits in the register list, should be between 0-16
    int set_registers[16];
    bool register_list_contains_rn = false;
    bool r15_in_transfer_list = (instruction >> 15) & 1;
    state_t temp_state = get_state();

    if (Rn == 15) {
        std::cerr << "r15 cannot be used as base register in BDT!" << "\n";
        return;
    }

    // get set registers in list
    // also determine if register list contains Rn
    for (int i = 0; i < 16; ++i) {
        if (register_list >> i & 1) {
            set_registers[num_registers] = i;
            num_registers++;
        }
        if (i == Rn) register_list_contains_rn = true;
    }
    
    if (load_psr && !r15_in_transfer_list) {
        set_state(USR);
        write_back = false;
    }
    // skip transfer if register list is all 0s
    // avoids indexing set_registers[-1] for decrement
    if (num_registers == 0) goto after_transfer;

    /*
     * Transfer registers or memory
     * One of the ugliest blocks of code I've ever written... yikes
     */
    if (load) { // load from addresses into register list
        if (up) { // addresses increment 
            for (int i = 0; i < num_registers; ++i) {
                if (pre_index) base += 4;
                if (load_psr && r15_in_transfer_list && (set_registers[i] == 15)) {
                    set_register(16, get_register(17)); // SPSR_<mode> is transferred to CPSR when r15 in loaded
                }
                set_register(set_registers[i], mem->read_u32(base));
                if (!pre_index) base += 4;
            }
        } else { // addresses decrement
            for (int i = num_registers - 1; i >= 0; --i) {
                if (pre_index) base -= 4;
                set_register(set_registers[i], mem->read_u32(base));
                if (!pre_index) base -= 4;
            }
        }
    } else { // store from address list into memory
        if (load_psr && r15_in_transfer_list) set_state(USR);
        if (up) { // addresses increment 
            for (int i = 0; i < num_registers; ++i) {
                if (pre_index) base += 4;
                mem->write_u32(base, get_register(set_registers[i]));
                if (!pre_index) base += 4;
            }
        } else { // addresses decrement
            for (int i = num_registers - 1; i >= 0; --i) {
                if (pre_index) base -= 4;
                mem->write_u32(base, get_register(set_registers[i]));
                if (!pre_index) base -= 4;
            }
        }
        if (load_psr && r15_in_transfer_list) {
            set_state(temp_state);
            write_back = false; // write back should not be enabled in this mechanism
        }
    }

    after_transfer:
    
    if (load_psr && !r15_in_transfer_list) set_state(temp_state);

    if (write_back || (load && register_list_contains_rn)) {
        set_register(Rn, base);
    }
}

inline void arm_7tdmi::single_data_swap(u32 instruction) {
    bool byte = util::get_instruction_subset(instruction, 22, 22);
    u32 Rn = util::get_instruction_subset(instruction, 19, 16); // base register
    u32 Rd = util::get_instruction_subset(instruction, 15, 12); // destination register
    u32 Rm = util::get_instruction_subset(instruction, 3, 0);   // source register

    if (Rn == 15 || Rd == 15 || Rm == 15) {
        std::cerr << "r15 can't be used as an operand in SWP!" << "\n";
        return;
    }

    u32 swap_address = get_register(Rn);
    if (byte) { // swap a byte
        u8 temp = mem->read_u8(swap_address);
        u8 source = get_register(Rm) & 0xFF; // bottom byte of source register
        mem->write_u8(swap_address, source);
        set_register(Rd, temp);
    } else { // swap a word
        u32 temp = mem->read_u32(swap_address);
        u32 source = get_register(Rm);
        mem->write_u32(swap_address, source);
        set_register(Rd, temp);
    }
}

inline void arm_7tdmi::software_interrupt(u32 instruction) {
    set_state(SVC);
    set_register(15, 0x08);
    set_register(17, get_register(16)); // put CPSR in SPSR_<svc>
}

void arm_7tdmi::move_shifted_register(u16 instruction) {
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); 
    u16 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset
    u16 shift_type = util::get_instruction_subset(instruction, 12, 11);
    u32 op1 = get_register(Rs);
    u8 carry_out = shift_register(offset5, op1, shift_type);
    set_register(Rd, op1);
    u8 carry = carry_out == 2 ? get_condition_code_flag(C) : carry_out;
    update_flags_logical(op1, carry);
}

void arm_7tdmi::add_sub(u16 instruction) {
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0);
    u16 Rn_offset3 = util::get_instruction_subset(instruction, 8, 6);
    bool immediate = util::get_instruction_subset(instruction, 10, 10) == 1;
    bool add = util::get_instruction_subset(instruction, 9, 9) == 0;
    u32 op1;
    u32 op2;
    u32 result;

    op1 = get_register(Rs);

    if (immediate) {
        op2 = Rn_offset3;
    } else {
        op2 = get_register(Rn_offset3);
    }

    if (add) {
        result = op1 + op2;
        update_flags_addition(op1, op2, result);
    } else {
        result = op1 - op2;
        update_flags_subtraction(op1, op2, result);
    }

    set_register(Rd, result);
}

void arm_7tdmi::move_immediate(u16 instruction) {
    u16 offset8 = util::get_instruction_subset(instruction, 7, 0);
    u16 Rd = util::get_instruction_subset(instruction, 10, 8);
    u16 opcode = util::get_instruction_subset(instruction, 12, 11);
    u32 result;
    u8 carry = get_condition_code_flag(C);
    u32 operand = get_register(Rd);

    switch (opcode) {
        case 0: // MOV
            result = offset8;
            set_register(Rd, result);
            update_flags_logical(result, carry);
            break;
        case 1: // CMP
            result = operand - offset8;
            update_flags_subtraction(operand, offset8, result);
            break;
        case 2: // ADD
            result = operand + offset8;
            set_register(Rd, result);
            update_flags_addition(operand, offset8, result);
            break;
        case 3: // SUB
            result = operand - offset8;
            set_register(Rd, result);
            update_flags_subtraction(operand, offset8, result);
            break;
    }
}

void arm_7tdmi::alu_thumb(u16 instruction) {
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); 
    u16 opcode = util::get_instruction_subset(instruction, 9, 6);
    u32 op1 = get_register(Rs);
    u32 op2 = get_register(Rd);
    u8 carry = get_condition_code_flag(C);
    u32 result;

    switch (opcode) {
        case 0b0000: // AND
            result = op1 & op2;
            set_register(Rd, result);
            update_flags_logical(result, carry);
            break;
        case 0b0001: // EOR
            result = op1 ^ op2;
            set_register(Rd, result);
            update_flags_logical(result, carry);
            break;
        case 0b0010: // LSL
            carry = shift_register(op1, op2, 0b00);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);
            break;
        case 0b0011: // LSR
            carry = shift_register(op1, op2, 0b01);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);
            break;
        case 0b0100: // ASR
            carry = shift_register(op1, op2, 0b10);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);
            break;
        case 0b0101: // ADC
            result = op1 + op2 + carry;
            set_register(Rd, result);
            update_flags_addition(op1, op2, result);
            break;
        case 0b0110: // SBC
            result = op2 - op1 - (~carry & 0x1); // Rd - Rs - NOT C-bit
            set_register(Rd, result);
            update_flags_subtraction(op2, op1, result);
            break;
        case 0b0111: // ASR
            carry = shift_register(op1, op2, 0b11);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);
            break;
        case 0b1000: // TST
            result = op1 & op2;
            update_flags_logical(result, carry);
            break;
        case 0b1001: // NEG
            result = op1 * -1;
            set_register(Rd, result);
            update_flags_logical(result, carry);
            break;
        case 0b1010: // CMP
            result = op2 - op1;
            update_flags_subtraction(op2, op1, result);
            break;
        case 0b1011: // CMN
            result = op2 + op1;
            update_flags_addition(op1, op2, result);
            break;
        case 0b1100: // ORR
            result = op2 | op1;
            set_register(Rd, result);
            update_flags_logical(result, carry);
            break;
        case 0b1101: // MUL
            result = op2 * op1;
            set_register(Rd, result);
            update_flags_addition(op1, op2, result);
            break;
        case 0b1110: // BIC
            result = op2 & ~op1;
            set_register(Rd, result);
            update_flags_logical(result, carry);
            break;
        case 0b1111: // MVN
            result = ~op1;
            set_register(Rd, result);
            update_flags_logical(result, carry);
            break;
        default:
            std::cerr << "Error: Invalid thumb ALU opcode" << "\n";
            return;
    }
}

void arm_7tdmi::hi_reg_ops(u16 instruction) {
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); 
    u16 opcode = util::get_instruction_subset(instruction, 9, 8);

    bool H1 = util::get_instruction_subset(instruction, 7, 7) == 0x1; // Hi operand flag 1
    bool H2 = util::get_instruction_subset(instruction, 6, 6) == 0x1; // Hi operand flag 2
    // access hi registers (need a 4th bit)
    if (H1) Rs |= 0b1000;
    if (H2) Rd |= 0b1000;

    u32 op1 = get_register(Rs);
    u32 op2 = get_register(Rd);
    u32 result;

    switch (opcode) {
        case 0b00: // ADD
            if (!H1 && !H2) {
                std::cerr << "Error: H1 = 0 and H2 = 0 for thumb ADD is not defined" << "\n";
                return;
            }

            result = op1 + op2;
            set_register(Rd, result);
            break;
        case 0b01: // CMP
            if (!H1 && !H2) {
                std::cerr << "Error: H1 = 0 and H2 = 0 for thumb CMP is not defined" << "\n";
                return;
            }

            result = op2 - op1;
            update_flags_subtraction(op2, op1, result);
            set_register(Rd, result);
            break;
        case 0b10: // MOV
            if (!H1 && !H2) {
                std::cerr << "Error: H1 = 0 and H2 = 0 for thumb MOV is not defined" << "\n";
                return;
            }

            set_register(Rd, op1);
            break;
        case 0b11: // BX
            if (H1) {
                std::cerr << "Error: H1 = 1 for thumb BX is not defined" << "\n";
                return;
            }

            if (Rs == 15) op1 += 4;
            set_register(15, op1);

            // swith to ARM mode if necessary
            if ((op1 & 1) == 0) {
                registers.r15 += 4; // continue at Rn + 4 in arm mode (skip following halfword)
                set_mode(ARM);
                registers.cpsr.bits.t = 0; // TBIT
            }
        break;
    }
}

void arm_7tdmi::pc_rel_load(u16 instruction) {
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); 
    u16 word8 = util::get_instruction_subset(instruction, 7, 0);

    word8 <<= 2; // assembler places #imm >> 2 in word8

    set_register(Rd, mem->read_u32(registers.r15 + word8));
}

void arm_7tdmi::load_store_reg(u16 instruction) {
    u16 Ro = util::get_instruction_subset(instruction, 8, 6); // offset register
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register

    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    bool byte = util::get_instruction_subset(instruction, 10, 10) == 1;

    u32 base = get_register(Rb);
    base += get_register(Ro); // add offset to base

    if (load) {
        if (byte) set_register(Rd, mem->read_u8(base));
        else set_register(Rd, mem->read_u32(base));
    } else { // store
        if (byte) mem->write_u8(base, get_register(Rd) & 0xFF);
        else mem->write_u32(base, get_register(Rd));
    }
}

void arm_7tdmi::load_store_signed_halfword(u16 instruction) {
    u16 Ro = util::get_instruction_subset(instruction, 8, 6); // offset register
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register

    bool H = util::get_instruction_subset(instruction, 11, 11) == 1; // H flag
    bool S = util::get_instruction_subset(instruction, 10, 10) == 1; // sign extended flag

    u32 base = get_register(Rb);
    base += get_register(Ro); // add offset to base

    if (!S && !H) { // store halfword
        mem->write_u16(base, get_register(Rd) & 0xFFFF);
    } else if (!S && H) { // load halfword
        u32 value = 0;
        value |= mem->read_u16(base);
        set_register(Rd, value);
    } else if (S && !H) { // load sign-extended byte
        u32 value = (u32) mem->read_u8(base);
        if (value & 0x80) value |= ~0b11111111; // bit 7 of byte is 1, so sign extend bits 31-8 of register
        set_register(Rd, value);
    } else { // load sign-extended halfword
        u32 value = (u32) mem->read_u16(base);
        if (value & 0x8000) value |= ~0b1111111111111111; // bit 15 of byte is 1, so sign extend bits 31-16 of register
        set_register(Rd, value);
    }
}

void arm_7tdmi::load_store_immediate(u16 instruction) {
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register
    u16 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset

    bool byte = util::get_instruction_subset(instruction, 12, 12) == 1;
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    
    if (!byte) offset5 <<= 2; // assembler places #imm >> 2 in word5 for word accesses

    u32 base = get_register(Rb);
    base += offset5; // add offset to base

    if (!load && !byte) { // store word
        mem->write_u32(base, get_register(Rd));
    } else if (load && !byte) { // load word
        set_register(Rd,  mem->read_u32(base));
    } else if (!load && byte) { // store byte
        mem->write_u8(base, get_register(Rd) & 0xFF);
    } else { // load byte
        set_register(Rd, mem->read_u8(base));
    }
}

void arm_7tdmi::load_store_halfword(u16 instruction) {
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register
    u16 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset

    offset5 <<= 1; // assembler places #imm >> 1 in word5 to ensure halfword alignment
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;

    u32 base = get_register(Rb);
    base += offset5; // add offset to base

    if (load) {
        set_register(Rd, mem->read_u16(base));
    } else { // store
        mem->write_u16(base, get_register(Rd) & 0xFFFF);
    }
}

void arm_7tdmi::sp_load_store(u16 instruction) {
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); // destination register
    u16 word8 = util::get_instruction_subset(instruction, 10, 6); // 8 bit immediate offset
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = get_register(13); // current stack pointer is base address
    base += word8; // add offset to base

    if (load) {
        set_register(Rd, mem->read_u32(base));
    } else { // store
        mem->write_u32(base, get_register(Rd));
    }
}

void arm_7tdmi::load_address(u16 instruction) {
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); // destination register
    u16 word8 = util::get_instruction_subset(instruction, 7, 0); // 8 bit immediate offset
    bool sp = util::get_instruction_subset(instruction, 11, 11) == 1; // stack pointer if true, else PC
    u32 base;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    if (sp) {
        base = get_register(13);
    } else { // pc
        base = get_register(15);
        base += 4; // pc will be 4 bytes greater than current address
        base += word8;
    }

    set_register(Rd, mem->read_u32(base));
}

void arm_7tdmi::add_offset_to_sp(u16 instruction) {
    u16 sword8 = util::get_instruction_subset(instruction, 6, 0); // 7 bit signed immediate value
    bool positive = util::get_instruction_subset(instruction, 7, 7) == 0; // sign bit of sword8

    sword8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = get_register(13); // base address at SP

    if (positive) base += sword8;
    else base -= sword8;

    set_register(13, base);
}

void arm_7tdmi::push_pop(u16 instruction) {
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    bool R = util::get_instruction_subset(instruction, 8, 8) == 1; // PC/LR bit
    u32 base = get_register(13); // base address at SP

    int num_registers = 0; // number of set bits in the register list, should be between 0-8
    int set_registers[8];

    // determine which registers are set
    for (int i = 0; i < 8; ++i) {
        if (instruction >> i) { // bit i is set in Rlist
            set_registers[num_registers] = i;
            num_registers++;
        }
    }

    if (!load) { // PUSH Rlist
        for (int i = 0; i < num_registers; ++i) {
            mem->write_u32(base, get_register(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
        }

        if (R) { // push LR
            mem->write_u32(base, get_register(14));
            base += 4; // increment stack pointer (4 bytes for word alignment)
        }
    } else { // POP Rlist
        for (int i = 0; i < num_registers; ++i) {
            set_register(set_registers[i], mem->read_u32(base));
            base -= 4; // decrement stack pointer (4 bytes for word alignment)
        }

        if (R) { // pop pc
            set_register(15, mem->read_u32(base));
            base -= 4; // decrement stack pointer (4 bytes for word alignment)
        }
    }
}

void arm_7tdmi::multiple_load_store(u16 instruction) {
    u16 Rb = util::get_instruction_subset(instruction, 10, 8); // base register
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    u32 base = get_register(Rb);

    int num_registers = 0; // number of set bits in the register list, should be between 0-16
    int set_registers[8];

    // determine which registers are set
    for (int i = 0; i < 8; ++i) {
        if (instruction >> i & 0x1) { // bit i is set in Rlist
            set_registers[num_registers] = i;
            num_registers++;
        }
    }

    if (load) { 
        for (int i = 0; i < num_registers; ++i) {
            set_register(set_registers[i], mem->read_u32(base));
            base += 4; // decrement stack pointer (4 bytes for word alignment)
        }
    } else { // store
        for (int i = 0; i < num_registers; ++i) {
            mem->write_u32(base, get_register(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
        }
    }

    // write back address into Rb
    set_register(Rb, base);
}

void arm_7tdmi::conditional_branch(u16 instruction) {
    u16 soffset8 = util::get_instruction_subset(instruction, 7, 0); // signed 8 bit offset
    condition_t condition = (condition_t) util::get_instruction_subset(instruction, 11, 8);
    u32 base = get_register(15);

    if (!condition_met(condition)) {
        increment_pc();
        return;
    }

    soffset8 <<= 1; // assembler places #imm >> 1 in word8 to ensure halfword alignment

    set_register(15, base + soffset8);

    // flush pipeline for refill
    pipeline_full = false;
}

void arm_7tdmi::software_interrupt_thumb(u16 instruction) {
    set_register(14, instruction + 2); // move the address of the next instruction into LR
    set_register(16, get_register(15)); // move CPSR to SPSR
    set_register(15, 0x8); // load the SWI vector address (0x8) into the PC

    // switch to ARM state and enter SVC mode
    set_mode(ARM);
    set_state(SVC);
    registers.cpsr.bits.state = SVC;
    registers.cpsr.bits.t = 0;
}

void arm_7tdmi::unconditional_branch(u16 instruction) {
    u16 offset11 = util::get_instruction_subset(instruction, 10, 0); // signed 11 bit offset
    u32 base = get_register(15);

    offset11 <<= 1; // assembler places #imm >> 1 in offset11 to ensure halfword alignment

    set_register(15, base + offset11);

    // flush pipeline for refill
    pipeline_full = false;
}

void arm_7tdmi::long_branch_link(u16 instruction) {
    u32 offset = util::get_instruction_subset(instruction, 10, 0); // long branch offset
    bool H = util::get_instruction_subset(instruction, 11, 11) == 1; // high/low offset bit
    u32 base;

    if (H) { // instruction 2
        base = get_register(14); // LR
        offset <<= 1;
        base += offset;

        // get address of next instruction and set bit 0
        u32 next_instruction_address = get_register(15) - 2;
        next_instruction_address |= 0x1;

        set_register(15, base);
        set_register(14, next_instruction_address); // next instruction in link register

        // flush pipeline for refill
        pipeline_full = false;
    } else { // instruction 1
        base = get_register(15); // PC
        offset <<= 12;
        base += offset;
        set_register(14, base); // resulting address stored in LR
        increment_pc();
    }
}

