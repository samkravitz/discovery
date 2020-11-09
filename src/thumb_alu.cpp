/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: thumb_alu.cpp
 * DATE: October 8th, 2020
 * DESCRIPTION: execution of thumb instructions
 */
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

    // cycles: 1S
    cycle(0, 1, 0);
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

    // cycles: 1S
    cycle(0, 1, 0);
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

    // cycles: 1S
    cycle(0, 1, 0);
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

    // cycles
    u8 n = 0;
    u8 s = 1;
    u8 i = 0;

    switch (opcode)
    {
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
            carry = barrel_shift(op1 & 0xFF, op2, 0b00);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            ++i; // 1I
            break;

        case 0b0011: // LSR
            carry = barrel_shift(op1 & 0xFF, op2, 0b01);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            ++i; // 1I
            break;

        case 0b0100: // ASR
            carry = barrel_shift(op1 & 0xFF, op2, 0b10);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            ++i; // 1I
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

        case 0b0111: // ROR
            carry = barrel_shift(op1 & 0xFF, op2, 0b11);
            set_register(Rd, op2);
            update_flags_logical(op2, carry);

            ++i; // 1I
            break;

        case 0b1000: // TST
            result = op1 & op2;
            update_flags_logical(result, carry);
            break;

        case 0b1001: // NEG
            result = 0 - op1;
            set_register(Rd, result);
            update_flags_subtraction(op2, op1, result);
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

    cycle(n, s, i);
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

    // cycles
    u8 n = 0;
    u8 s = 1;
    u8 i = 0;

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
                pipeline_full = false;

                // +1S, +1N if r15 is destination
                ++s;
                ++n;
            }
            
            else
                increment_pc();
        
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
                pipeline_full = false;

                // +1S, +1N if r15 is destination
                ++s;
                ++n;
            }
            
            else
                increment_pc();

            break;
        case 0b11: // BX
            if (H1)
            {
                std::cerr << "Error: H1 = 1 for thumb BX is not defined" << "\n";
                return;
            }

            // swith to ARM mode if necessary
            if ((op1 & 1) == 0)
            {
                // align to word boundary
                op1 &= ~3;
                set_register(15, op1);
                set_mode(ARM);
            }

            else
            {
                // clear bit 0
                op1 &= ~1;
            }

            set_register(15, op1);

            // flush pipeline for refill
            pipeline_full = false;

            ++s;
            ++n;
        break;
    }

    // cycles:
    // 1S for ADD/MOV/CMP
    // 2S + 1N for Rd = 15 or BX
    cycle(n, s, i);
}

void arm_7tdmi::pc_rel_load(u16 instruction)
{
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); 
    u16 word8 = util::get_instruction_subset(instruction, 7, 0);
    u32 base = get_register(15);
    base &= ~2; // clear bit 1 for word alignment

    word8 <<= 2; // assembler places #imm >> 2 in word8
    base += word8;

    set_register(Rd, read_u32(base, true));
    
    // cycles: 1S + 1N + 1I
    cycle(1, 1, 1);
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

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    if (load)
    {
        if (byte)
            set_register(Rd, read_u8(base));
        else
            set_register(Rd, read_u32(base, true));
        
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        if (byte)
            write_u8(base, get_register(Rd) & 0xFF);
        else
            write_u32(base, get_register(Rd));

        n = 2;    
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    cycle(n, s, i);
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

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    // store halfword
    if (!S && !H)
    {
        write_u16(base, get_register(Rd) & 0xFFFF);
        n = 2;
    }
    
    // load halfword
    else if (!S && H)
    {
        u32 value = read_u16(base, false);
        set_register(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }
    
    // load sign-extended byte
    else if (S && !H)
    {
        u32 value = read_u8(base);
        if (value & 0x80)
            value |= 0xFFFFFF00; // bit 7 of byte is 1, so sign extend bits 31-8 of register
        set_register(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }
    
    // load sign-extended halfword
    else
    {
        u32 value = read_u16(base, true);
        set_register(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    cycle(n, s, i);
}

void arm_7tdmi::load_store_immediate(u16 instruction)
{
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register
    u16 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset

    bool byte = util::get_instruction_subset(instruction, 12, 12) == 1;
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    
    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    if (!byte)
        offset5 <<= 2; // assembler places #imm >> 2 in word5 for word accesses

    u32 base = get_register(Rb);
    base += offset5; // add offset to base

    // store word
    if (!load && !byte)
    { 
        write_u32(base, get_register(Rd));
        n = 2;
    }
    
    // load word
    else if (load && !byte)
    {
        set_register(Rd,  read_u32(base, true));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store byte
    else if (!load && byte)
    {
        write_u8(base, get_register(Rd) & 0xFF);
        n = 2;
    }
    
    else
    { // load byte
        set_register(Rd, read_u8(base));
        n = 1;
        s = 1;
        i = 1;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    cycle(n, s, i);
}

void arm_7tdmi::load_store_halfword(u16 instruction)
{
    u16 Rb = util::get_instruction_subset(instruction, 5, 3); // base register
    u16 Rd = util::get_instruction_subset(instruction, 2, 0); // destination register
    u16 offset5 = util::get_instruction_subset(instruction, 10, 6); // 5 bit immediate offset

    offset5 <<= 1; // assembler places #imm >> 1 in word5 to ensure halfword alignment
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    u32 base = get_register(Rb);
    base += offset5; // add offset to base

    if (load)
    {
        set_register(Rd, read_u16(base, false));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        write_u16(base, get_register(Rd) & 0xFFFF);
        n = 2;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    cycle(n, s, i);
}

void arm_7tdmi::sp_load_store(u16 instruction)
{
    u16 Rd = util::get_instruction_subset(instruction, 10, 8); // destination register
    u16 word8 = util::get_instruction_subset(instruction, 7, 0); // 8 bit immediate offset
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = get_register(13); // current stack pointer is base address
    base += word8; // add offset to base

    if (load)
    {
        set_register(Rd, read_u32(base, true));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        write_u32(base, get_register(Rd));
        n = 2;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    cycle(n, s, i);
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

    // cycles: 1S
    cycle(0, 1, 0);
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
    
    // cycles: 1S
    cycle(0, 1, 0);
}

void arm_7tdmi::push_pop(u16 instruction)
{
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    bool R = util::get_instruction_subset(instruction, 8, 8) == 1; // PC/LR bit
    u32 base = get_register(13); // base address at SP

    int num_registers = 0; // number of set bits in the register list, should be between 0-8
    int set_registers[8];

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    // determine which registers are set
    for (int i = 0; i < 8; ++i)
    {
        if (instruction >> i & 0x1) // bit i is set in Rlist
        {
            set_registers[num_registers] = i;
            num_registers++;
        }
    }

    // special case - empty registers list
    // if (num_registers == 0)
    // {
    //     if (load) // load r15
    //     { 
    //         set_register(15, read_u32(base, false));
    //         pipeline_full = false;
    //     }

    //     else // store r15
    //     {
    //         write_u32(base, registers.r15 + 4);
    //         increment_pc();
    //     }

    //     // store Rb = Rb +/- 0x40
    //     set_register(13, base + 0x4);

    //     return;
    // }

    if (!load) // PUSH Rlist
    {
        n = 2;
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
            ++s;
        }

        if (R) // push LR
        {
            write_u32(base, get_register(14));
            // base -= 4; // increment stack pointer (4 bytes for word alignment)
            ++s;
        }

        increment_pc();
    }
    
    else // POP Rlist
    {
        n = 1;
        i = 1;
        for (int i = 0; i < num_registers; ++i)
        {
            set_register(set_registers[i], read_u32(base, false));
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            ++s;
        }

        if (R) // pop pc
        {
            set_register(15, read_u32(base, false) & ~1); // guaruntee halfword alignment
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            ++s;
            ++n;
        }
        
        else
        {
            increment_pc();
        }

        // write base back into sp
        set_register(13, base);
    }

    // cycles:
    // nS + 1N + 1I (POP)
    // (n + 1)S + 2N + 1I (POP PC)
    // (n-1)S + 2N (PUSH).
    cycle(n, s, i);
}

void arm_7tdmi::multiple_load_store(u16 instruction)
{
    u16 Rb = util::get_instruction_subset(instruction, 10, 8); // base register
    bool load = util::get_instruction_subset(instruction, 11, 11) == 1;
    u32 base = get_register(Rb);

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

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

    // empty Rlist, Rb = Rb + 0x40
    if (num_registers == 0)
    {
        if (load) // load r15
        { 
            set_register(15, read_u32(base, false));
            pipeline_full = false;
        }

        else // store r15
        {
            write_u32(base, registers.r15 + 4);
            increment_pc();
        }

        // store Rb = Rb +/- 0x40
        set_register(Rb, base + 0x40);

        return;
    }

    if (load)
    { 
        for (int i = 0; i < num_registers; ++i)
        {
            set_register(set_registers[i], read_u32(base, false));
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            ++s;
        }

        ++n;
        ++i;
    }
    
    else // store
    {
        for (int i = 0; i < num_registers; ++i)
        {
            write_u32(base, get_register(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
            ++s;
        }

        n = 2;
    }

    // write back address into Rb
    set_register(Rb, base);

    // cycles:
    // nS + 1N + 1I for LDM
    // (n - 1)S + 2N for STM
    cycle(n, s, i);
}

void arm_7tdmi::conditional_branch(u16 instruction)
{
    u16 soffset8 = util::get_instruction_subset(instruction, 7, 0); // signed 8 bit offset
    condition_t condition = (condition_t) util::get_instruction_subset(instruction, 11, 8);
    u32 base = get_register(15);
    u32 jump_address;
    if (!condition_met(condition))
    {
        // 1S
        cycle(0, 1, 0);
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

    set_register(15, jump_address);

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    cycle(1, 2, 0);
}

void arm_7tdmi::software_interrupt_thumb(u16 instruction)
{
    std::cout << "thumb SWI:  " << (instruction & 0xFF) << "\n";

    // bits 7 - 0 determine which interrupt
    switch (instruction & 0xFF)
    {
        case 0x0:
            swi_softReset();
            break;
        case 0x1:
            swi_registerRamReset();
            break;
        case 0x6:
            swi_division();
            break;
        case 0x8:
            swi_sqrt();
            break;
        case 0xA:
            swi_arctan2();
            break;
        case 0xB:
            swi_cpuSet();
            break;
        case 0xF:
            swi_objAffineSet();
            break;
        case 0x10:
            swi_bitUnpack();
            break;
        default:
            std::cout << "Unknown SWI code: " << std::hex << (instruction & 0xFF) << "\n";
    }

    // TODO - handle swi through bios ?
    // set_state(SVC);
    // set_register(14, get_register(15) - 2);
    // update_spsr(get_register(16), false);
    // set_mode(ARM);
    // set_register(15, 0x08);
    // pipeline_full = false;
    // return;

    // cycles: 2S + 1N
    cycle(1, 2, 0);
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

    set_register(15, jump_address);

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    cycle(1, 2, 0);
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

        set_register(15, base);
        set_register(14, next_instruction_address); // next instruction in link register

        // flush pipeline for refill
        pipeline_full = false;
        
        // cycles: 3S + 1N
        cycle(1, 3, 0);
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
    }
}

