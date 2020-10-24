/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: thumb_alu.cpp
 * DATE: October 8th, 2020
 * DESCRIPTION: execution of thumb instructions
 */
#include <iostream>
#include "common/util.h"
#include "arm_7tdmi.h"

void arm_7tdmi::move_shifted_register(u16 instruction)
{
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); 
    u32 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset
    u16 shift_type = util::get_instruction_subset(instruction, 12, 11);
    u32 op1 = get_register(Rs);

    // encodings of LSR #0, ASR #0, and ROR #0 should be interpreted as #LSR #2, ASR #32, and ROR #32
    if (offset5 == 0 && shift_type != 0) // shift_type == 0 is LSL
    {
        if (shift_type == 0b11) // rotate right extended
            offset5 = 0xFFFFFFFF;
        else // LSR #32 or ASR #32
            offset5 = 32;
    }

    u8 carry_out = barrel_shift(offset5, op1, shift_type);
    set_register(Rd, op1);
    update_flags_logical(op1, carry_out);

    cycle(registers.r15, 's'); // 1S
}

void arm_7tdmi::add_sub(u16 instruction)
{
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0);
    u16 Rn_offset3 = util::get_instruction_subset(instruction, 8, 6);
    bool immediate = util::get_instruction_subset(instruction, 10, 10) == 1;
    bool add = util::get_instruction_subset(instruction, 9, 9) == 0;
    u32 op1;
    u32 op2;
    u32 result;

    op1 = get_register(Rs);

    if (immediate)
        op2 = Rn_offset3;
    else
        op2 = get_register(Rn_offset3);

    if (add)
    {
        result = op1 + op2;
        update_flags_addition(op1, op2, result);
    }
    
    else
    {
        result = op1 - op2;
        update_flags_subtraction(op1, op2, result);
    }

    set_register(Rd, result);

    cycle(registers.r15, 's'); // 1S
}

void arm_7tdmi::move_immediate(u16 instruction)
{
    u16 offset8 = util::get_instruction_subset(instruction, 7, 0);
    u16 Rd = util::get_instruction_subset(instruction, 10, 8);
    u16 opcode = util::get_instruction_subset(instruction, 12, 11);
    u32 result;
    u8 carry = get_condition_code_flag(C);
    u32 operand = get_register(Rd);

    switch (opcode)
    {
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

    cycle(registers.r15, 's'); // 1S
}

void arm_7tdmi::alu_thumb(u16 instruction)
{
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); 
    u16 opcode = util::get_instruction_subset(instruction, 9, 6);
    u32 op1 = get_register(Rs);
    u32 op2 = get_register(Rd);
    u8 carry = get_condition_code_flag(C);
    u32 result;

    switch (opcode)
    {
        case 0b0000: // AND
            result = op1 & op2;
            set_register(Rd, result);
            update_flags_logical(result, carry);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b0001: // EOR
            result = op1 ^ op2;
            set_register(Rd, result);
            update_flags_logical(result, carry);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b0010: // LSL
            carry = barrel_shift(op1 & 0xFF, op2, 0b00);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            cycle(registers.r15, 'i'); // 1I
            cycle(registers.r15 + 2, 's'); // 1S

            break;

        case 0b0011: // LSR
            carry = barrel_shift(op1 & 0xFF, op2, 0b01);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            cycle(registers.r15, 'i'); // 1I
            cycle(registers.r15 + 2, 's'); // 1S

            break;

        case 0b0100: // ASR
            carry = barrel_shift(op1 & 0xFF, op2, 0b10);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            cycle(registers.r15, 'i'); // 1I
            cycle(registers.r15 + 2, 's'); // 1S

            break;

        case 0b0101: // ADC
            result = op1 + op2 + carry;
            set_register(Rd, result);
            update_flags_addition(op1, op2, result);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b0110: // SBC
            result = op2 - op1 - (~carry & 0x1); // Rd - Rs - NOT C-bit
            set_register(Rd, result);
            update_flags_subtraction(op2, op1, result);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b0111: // ROR
            carry = barrel_shift(op1 & 0xFF, op2, 0b11);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            cycle(registers.r15, 'i'); // 1I
            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1000: // TST
            result = op1 & op2;
            update_flags_logical(result, carry);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1001: // NEG
            result = op1 * -1;
            set_register(Rd, result);
            update_flags_logical(result, carry);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1010: // CMP
            result = op2 - op1;
            update_flags_subtraction(op2, op1, result);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1011: // CMN
            result = op2 + op1;
            update_flags_addition(op1, op2, result);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1100: // ORR
            result = op2 | op1;
            set_register(Rd, result);
            update_flags_logical(result, carry);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1101: // MUL
            result = op2 * op1;
            set_register(Rd, result);
            update_flags_addition(op1, op2, result);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1110: // BIC
            result = op2 & ~op1;
            set_register(Rd, result);
            update_flags_logical(result, carry);

            cycle(registers.r15, 's'); // 1S

            break;

        case 0b1111: // MVN
            result = ~op1;
            set_register(Rd, result);
            update_flags_logical(result, carry);

            cycle(registers.r15, 's'); // 1S

            break;

        default:
            std::cerr << "Error: Invalid thumb ALU opcode" << "\n";
            return;
    }
}

void arm_7tdmi::hi_reg_ops(u16 instruction)
{
    u16 Rs = util::get_instruction_subset(instruction, 5, 3);
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); 
    u16 opcode = util::get_instruction_subset(instruction, 9, 8);

    bool H1 = util::get_instruction_subset(instruction, 7, 7) == 0x1; // Hi operand flag 1
    bool H2 = util::get_instruction_subset(instruction, 6, 6) == 0x1; // Hi operand flag 2

    // access hi registers (need a 4th bit)
    if (H2) Rs |= 0b1000;
    if (H1) Rd |= 0b1000;

    u32 op1 = get_register(Rs);
    u32 op2 = get_register(Rd);
    u32 result;

    switch (opcode)
    {
        case 0b00: // ADD
            if (!H1 && !H2)
            {
                std::cerr << "Error: H1 = 0 and H2 = 0 for thumb ADD is not defined" << "\n";
                return;
            }

            // clear bit 0 when destination is r15
            if (Rd == 15)
                op1 &= ~0x1;

            result = op1 + op2;
            set_register(Rd, result);

            if (Rd == 15)
            {
                cycle(registers.r15, 'n'); // + 1N cycles for r15

                pipeline_full = false;

                // + 2S cycles if r15 is destination
                cycle(registers.r15, 's'); // 2S
                cycle(registers.r15 + 2, 's');
            }
            
            else
            {
                increment_pc();
                cycle(registers.r15, 's'); // 1S
            }
        
            break;

        case 0b01: // CMP
            if (!H1 && !H2)
            {
                std::cerr << "Error: H1 = 0 and H2 = 0 for thumb CMP is not defined" << "\n";
                return;
            }

            result = op2 - op1;
            update_flags_subtraction(op2, op1, result);
            increment_pc();

            cycle(registers.r15, 's'); // 1S

            break;
        case 0b10: // MOV
            if (!H1 && !H2)
            {
                std::cerr << "Error: H1 = 0 and H2 = 0 for thumb MOV is not defined" << "\n";
                return;
            }

            // clear bit 0 when destination is r15
            if (Rd == 15)
                op1 &= ~0x1;

            result = op1;
            set_register(Rd, result);

            if (Rd == 15)
            {
                cycle(registers.r15, 'n'); // + 1N cycles for r15

                pipeline_full = false;

                // + 2S cycles if r15 is destination
                cycle(registers.r15, 's'); // 2S
                cycle(registers.r15 + 2, 's');
            }
            
            else
            {
                increment_pc();
                cycle(registers.r15, 's'); // 1S
            }

            break;
        case 0b11: // BX
            if (H1)
            {
                std::cerr << "Error: H1 = 1 for thumb BX is not defined" << "\n";
                return;
            }

            // clear bit 0 if Rs is 15
            if (Rs == 15)
            {
                op1 &= ~1;
                op1 += 4;
            }
            
            cycle(registers.r15, 'n'); // 1N

            set_register(15, op1);

            // swith to ARM mode if necessary
            if ((op1 & 1) == 0)
            {
                // registers.r15 += 4; // continue at Rn + 4 in arm mode (skip following halfword)
                set_mode(ARM);
            } else

            {
                // clear bit 0
                registers.r15 &= ~1;
            }

            // flush pipeline for refill
            pipeline_full = false;

            cycle(registers.r15, 's'); // 2S
            cycle(registers.r15 + 2, 's');
        break;
    }
}

void arm_7tdmi::pc_rel_load(u16 instruction)
{
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); 
    u16 word8 = util::get_instruction_subset(instruction, 7, 0);
    u32 base = get_register(15);
    base &= ~2; // clear bit 1 for word alignment

    word8 <<= 2; // assembler places #imm >> 2 in word8
    base += word8;

    cycle(base, 'i'); // 1I
    set_register(Rd, read_u32(base, true));
    cycle(registers.r15, 'n'); // 1N
    cycle(registers.r15 + 2, 's'); // 1S
}

void arm_7tdmi::load_store_reg(u16 instruction)
{
    u16 Ro = util::get_instruction_subset(instruction, 8, 6); // offset register
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register

    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    bool byte = util::get_instruction_subset(instruction, 10, 10) == 1;

    u32 base = get_register(Rb);
    base += get_register(Ro); // add offset to base

    if (load)
    {
        cycle(registers.r15, 'n'); // 1N
        if (byte)
            set_register(Rd, read_u8(base));
        else
        {
            set_register(Rd, read_u32(base, true));
            cycle(base, 'i'); // + 1I cycles for non byte load
        }
        cycle(base, 'n'); // 1N
    }
    
    // store
    else
    {
        cycle(registers.r15, 'n'); // 1N
        if (byte)
            write_u8(base, get_register(Rd) & 0xFF);
        else
            write_u32(base, get_register(Rd));
        cycle(base, 'n'); // 1N
    }
}

void arm_7tdmi::load_store_signed_halfword(u16 instruction)
{
    u16 Ro = util::get_instruction_subset(instruction, 8, 6); // offset register
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register

    bool H = util::get_instruction_subset(instruction, 11, 11) == 1; // H flag
    bool S = util::get_instruction_subset(instruction, 10, 10) == 1; // sign extended flag

    u32 base = get_register(Rb);
    base += get_register(Ro); // add offset to base

    // store halfword
    if (!S && !H)
    {
        cycle(registers.r15, 'n');
        write_u16(base, get_register(Rd) & 0xFFFF);
        cycle(registers.r15, 's');
    }
    
    // load halfword
    else if (!S && H)
    {
        cycle(registers.r15, 's');
        u32 value = read_u16(base, false);
        cycle(base, 'i');
        set_register(Rd, value);
        cycle(registers.r15 + 2, 's'); // + 1S cycles for non byte load
    }
    
    // load sign-extended byte
    else if (S && !H)
    {
        cycle(registers.r15, 'n');
        u32 value = read_u8(base);
        cycle(base, 'i');
        if (value & 0x80)
            value |= 0xFFFFFF00; // bit 7 of byte is 1, so sign extend bits 31-8 of register
        set_register(Rd, value);
        cycle(registers.r15 + 2, 's'); // + 1S cycles for non byte load
    }
    
    // load sign-extended halfword
    else
    {
        cycle(registers.r15, 'n');
        u32 value = read_u16(base, true);
        cycle(base, 'i');
        set_register(Rd, value);
        cycle(registers.r15 + 2, 's'); // + 1S cycles for non byte load
    }
}

void arm_7tdmi::load_store_immediate(u16 instruction)
{
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register
    u16 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset

    bool byte = util::get_instruction_subset(instruction, 12, 12) == 1;
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    
    if (!byte)
        offset5 <<= 2; // assembler places #imm >> 2 in word5 for word accesses

    u32 base = get_register(Rb);
    base += offset5; // add offset to base

    // store word
    if (!load && !byte)
    { 
        cycle(registers.r15, 'n'); // 1N
        write_u32(base, get_register(Rd));
        cycle(base, 'n'); // 1N
    }
    
    // load word
    else if (load && !byte)
    {
        cycle(registers.r15, 's'); // 1S
        cycle(registers.r15, 'i'); // 1I
        set_register(Rd,  read_u32(base, true));
        cycle(registers.r15 + 2, 'n');
    }
    
    // store byte
    else if (!load && byte)
    {
        cycle(registers.r15, 'n'); // 1N
        write_u8(base, get_register(Rd) & 0xFF);
        cycle(base, 'n'); // 1N
    }
    
    else
    { // load byte
        cycle(registers.r15, 'n'); // 1N
        set_register(Rd, read_u8(base));
        cycle(registers.r15, 'i'); // 1I
        cycle(registers.r15 + 2, 's'); // 1S
    }
}

void arm_7tdmi::load_store_halfword(u16 instruction)
{
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register
    u16 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset

    offset5 <<= 1; // assembler places #imm >> 1 in word5 to ensure halfword alignment
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;

    u32 base = get_register(Rb);
    base += offset5; // add offset to base

    if (load)
    {
        cycle(registers.r15, 's'); // + 1S cycles for load
        set_register(Rd, read_u16(base, false));
    }
    
    // store
    else
    {
        write_u16(base, get_register(Rd) & 0xFFFF);
    }

    cycle(base, 'n'); // 2N
    cycle(base, 'n');
}

void arm_7tdmi::sp_load_store(u16 instruction)
{
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); // destination register
    u16 word8 = util::get_instruction_subset(instruction, 7, 0); // 8 bit immediate offset
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = get_register(13); // current stack pointer is base address
    base += word8; // add offset to base

    if (load)
    {
        cycle(base, 's'); // + 1S cycles for load
        set_register(Rd, read_u32(base, true));
    }
    
    // store
    else
    {
        write_u32(base, get_register(Rd));
    }

    cycle(base, 'n'); // 2N
    cycle(base, 'n');
}

void arm_7tdmi::load_address(u16 instruction)
{
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); // destination register
    u16 word8 = util::get_instruction_subset(instruction, 7, 0); // 8 bit immediate offset
    bool sp = util::get_instruction_subset(instruction, 11, 11) == 1; // stack pointer if true, else PC
    u32 base;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    if (sp)
    {
        base = get_register(13);
    }
    
    else
    { // pc
        base = get_register(15);
        base &= ~2; // force bit 1 of PC to 0
    }

    base += word8;

    set_register(Rd, base);

    cycle(base, 's'); // 1S
}

void arm_7tdmi::add_offset_to_sp(u16 instruction)
{
    u16 sword8 = util::get_instruction_subset(instruction, 6, 0); // 7 bit signed immediate value
    bool positive = util::get_instruction_subset(instruction, 7, 7) == 0; // sign bit of sword8

    sword8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = get_register(13); // base address at SP

    if (positive)
        base += sword8;
    else
        base -= sword8;

    set_register(13, base);
    cycle(base, 's'); // 1S
}

void arm_7tdmi::push_pop(u16 instruction)
{
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    bool R = util::get_instruction_subset(instruction, 8, 8) == 1; // PC/LR bit
    u32 base = get_register(13); // base address at SP

    int num_registers = 0; // number of set bits in the register list, should be between 0-8
    int set_registers[8];

    // determine which registers are set
    for (int i = 0; i < 8; ++i)
    {
        if (instruction >> i & 0x1) // bit i is set in Rlist
        {
            set_registers[num_registers] = i;
            num_registers++;
        }
    }

    cycle(base, 'n'); // 1N

    if (!load) // PUSH Rlist
    {
        // get final sp value
        base -= 4 * num_registers;
        if (R)
            base -= 4;

        // write base back into sp
        set_register(13, base);
        
        // push registers
        for (int i = 0; i < num_registers; ++i)
        {
            write_u32(base, get_register(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
            cycle(base, 's'); // 1S
        }

        if (R) // push LR
        {
            write_u32(base, get_register(14));
            // base -= 4; // increment stack pointer (4 bytes for word alignment)
            cycle(base, 's'); // 1S
        }

        increment_pc();

    }
    
    else // POP Rlist
    {
        for (int i = 0; i < num_registers; ++i)
        {
            set_register(set_registers[i], read_u32(base, false));
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            cycle(base, 's');
        }

        if (R) // pop pc
        {
            set_register(15, read_u32(base, false) & ~1); // guaruntee halfword alignment
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            cycle(base, 's');
        }
        
        else
        {
            increment_pc();
        }

        // write base back into sp
        set_register(13, base);
    }
}

void arm_7tdmi::multiple_load_store(u16 instruction)
{
    u16 Rb = util::get_instruction_subset(instruction, 10, 8); // base register
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    u32 base = get_register(Rb);

    int num_registers = 0; // number of set bits in the register list, should be between 0-16
    int set_registers[8];

    // determine which registers are set
    for (int i = 0; i < 8; ++i)
    {
        if (instruction >> i & 0x1) // bit i is set in Rlist
        {
            set_registers[num_registers] = i;
            num_registers++;
        }
    }

    cycle(base, 'i');

    // empty Rlist, Rb = Rb + 0x40
    if (num_registers == 0)
    {
        set_register(Rb, get_register(Rb) + 0x40);
        return;
    }

    if (load)
    { 
        for (int i = 0; i < num_registers; ++i)
        {
            set_register(set_registers[i], read_u32(base, false));
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            cycle(base, 's');
        }

        cycle(base, 's');
        cycle(base, 's');
    }
    
    else // store
    {
        for (int i = 0; i < num_registers; ++i)
        {
            write_u32(base, get_register(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
            cycle(base, 's');
        }
    }

    // write back address into Rb
    set_register(Rb, base);
}

void arm_7tdmi::conditional_branch(u16 instruction)
{
    u16 soffset8 = util::get_instruction_subset(instruction, 7, 0); // signed 8 bit offset
    condition_t condition = (condition_t) util::get_instruction_subset(instruction, 11, 8);
    u32 base = get_register(15);
    u32 jump_address;
    if (!condition_met(condition))
    {
        cycle(base, 's');
        increment_pc();
        return;
    }

    soffset8 <<= 1; // assembler places #imm >> 1 in word8 to ensure halfword alignment

    // if soffset8 is negative signed, convert two's complement and subtract
    if (soffset8 >> 8)
    {
        // flip bits and add 1
        u8 twos_comp = ~soffset8;
        twos_comp += 1;
        jump_address = base - twos_comp;
    }
    
    else
    {
        jump_address = base + soffset8;
    }

    cycle(base, 's');
    set_register(15, jump_address);

    cycle(base, 's');
    cycle(base, 's');

    // flush pipeline for refill
    pipeline_full = false;
}

void arm_7tdmi::software_interrupt_thumb(u16 instruction)
{
    // set_register(14, instruction + 2); // move the address of the next instruction into LR
    // set_register(16, get_register(15)); // move CPSR to SPSR
    // set_register(15, 0x8); // load the SWI vector address (0x8) into the PC

    std::cout << "software interrupt thumb\n";
    // bits 7 - 0 determine which interrupt
    switch (instruction & 0xFF)
    {
        case 0x00:
            swi_softreset();
            break;
        case 0x06:
            swi_division();
            break;

        default:
            std::cout << "Unknown SWI code: " << std::hex << (instruction >> 16 & 0xFF) << "\n";
    }
    //exit(0);
    // switch to ARM state and enter SVC mode
    // set_mode(ARM);
    // set_state(SVC);
    // registers.cpsr.bits.state = SVC;
    // registers.cpsr.bits.t = 0;
}

void arm_7tdmi::unconditional_branch(u16 instruction)
{
    u16 offset11 = util::get_instruction_subset(instruction, 10, 0); // signed 11 bit offset
    u32 base = get_register(15);
    u32 jump_address;

    offset11 <<= 1; // assembler places #imm >> 1 in offset11 to ensure halfword alignment

    // if offset11 is negative signed, convert two's complement and subtract
    if (offset11 >> 11)
    {
        // flip bits and add 1
        u16 twos_comp = ~offset11;
        
        // clear top 5 bits of twos complement
        twos_comp <<= 5;
        twos_comp >>= 5;
        
        twos_comp += 1;
        jump_address = base - twos_comp;
    }
    
    else
    {
        jump_address = base + offset11;
    }

    cycle(base, 's');
    set_register(15, jump_address);

    cycle(base, 's');
    cycle(base, 's');

    // flush pipeline for refill
    pipeline_full = false;
}

void arm_7tdmi::long_branch_link(u16 instruction)
{
    u32 offset = util::get_instruction_subset(instruction, 10, 0); // long branch offset
    bool H = util::get_instruction_subset(instruction, 11, 11) == 1; // high/low offset bit
    u32 base;

    if (H) // instruction 2
    {
        base = get_register(14); // LR
        offset <<= 1;
        base += offset;

        // get address of next instruction and set bit 0
        u32 next_instruction_address = get_register(15) - 2;
        next_instruction_address |= 0x1;

        cycle(base, 's');

        set_register(15, base);
        set_register(14, next_instruction_address); // next instruction in link register

        // flush pipeline for refill
        pipeline_full = false;
        
        cycle(base, 's');
        cycle(base, 's');
    }
    
    else // instruction 1
    {
        base = get_register(15); // PC
        offset <<= 12;

        if (offset >> 22)
        {
            // flip bits and add 1
            u32 twos_comp = ~offset;

            // clear leading 1s that are guarunteed to be there from the u32 type
            twos_comp <<= 10;
            twos_comp >>= 10;

            twos_comp += 1;
            base -= twos_comp;
        }
        
        else
        {
            base += offset;
        }

        set_register(14, base); // resulting address stored in LR
        increment_pc();

        cycle(base, 's');
    }
}

