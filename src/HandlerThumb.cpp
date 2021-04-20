/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: HandlerThumb.cpp
 * DATE: October 8th, 2020
 * DESCRIPTION: execution of thumb instructions
 */
#include "Arm7Tdmi.h"

void Arm7Tdmi::moveShiftedRegister(u16 instruction)
{
    u16 Rs         = util::bitseq<5, 3>(instruction);
    u16 Rd         = util::bitseq<2, 0>(instruction); 
    u32 offset5    = util::bitseq<10, 6>(instruction); // 5 bit immediate offset
    u16 shift_type = util::bitseq<12, 11>(instruction);
    u32 op1 = getRegister(Rs);

    // encodings of LSR #0, ASR #0, and ROR #0 should be interpreted as #LSR #2, ASR #32, and ROR #32
    if (offset5 == 0 && shift_type != 0) // shift_type == 0 is LSL
    {
        if (shift_type == 0b11) // rotate right extended
            offset5 = 0xFFFFFFFF;
        else // LSR #32 or ASR #32
            offset5 = 32;
    }

    u8 carry_out = barrelShift(offset5, op1, shift_type);
    setRegister(Rd, op1);
    updateFlagsLogical(op1, carry_out);

    // cycles: 1S
    tick(0, 1, 0);
}

void Arm7Tdmi::addSubtract(u16 instruction)
{
    u16 Rs         = util::bitseq<5, 3>(instruction);
    u16 Rd         = util::bitseq<2, 0>(instruction);
    u16 Rn_offset3 = util::bitseq<8, 6>(instruction);
    bool immediate = util::bitseq<10, 10>(instruction) == 1;
    bool add       = util::bitseq<9, 9>(instruction) == 0;
    u32 op1, op2, result;

    op1 = getRegister(Rs);

    if (immediate)
        op2 = Rn_offset3;
    else
        op2 = getRegister(Rn_offset3);

    if (add)
    {
        result = op1 + op2;
        updateFlagsAddition(op1, op2, result);
    }
    
    else
    {
        result = op1 - op2;
        updateFlagsSubtraction(op1, op2, result);
    }

    setRegister(Rd, result);

    // cycles: 1S
    tick(0, 1, 0);
}

void Arm7Tdmi::moveImmediate(u16 instruction)
{
    u16 offset8 = util::bitseq<7, 0>(instruction);
    u16 Rd      = util::bitseq<10, 8>(instruction);
    u16 opcode  = util::bitseq<12, 11>(instruction);
    u32 result;
    u8 carry = getConditionCodeFlag(ConditionFlag::C);
    u32 operand = getRegister(Rd);

    switch (opcode)
    {
        case 0: // MOV
            result = offset8;
            setRegister(Rd, result);
            updateFlagsLogical(result, carry);
            break;
        case 1: // CMP
            result = operand - offset8;
            updateFlagsSubtraction(operand, offset8, result);
            break;
        case 2: // ADD
            result = operand + offset8;
            setRegister(Rd, result);
            updateFlagsAddition(operand, offset8, result);
            break;
        case 3: // SUB
            result = operand - offset8;
            setRegister(Rd, result);
            updateFlagsSubtraction(operand, offset8, result);
            break;
    }

    // cycles: 1S
    tick(0, 1, 0);
}

void Arm7Tdmi::aluThumb(u16 instruction)
{
    u16 Rs     = util::bitseq<5, 3>(instruction);
    u16 Rd     = util::bitseq<2, 0>(instruction); 
    u16 opcode = util::bitseq<9, 6>(instruction);
    u32 op1    = getRegister(Rs);
    u32 op2    = getRegister(Rd);
    u8 carry   = getConditionCodeFlag(ConditionFlag::C);
    u32 result;

    // cycles
    u8 n = 0;
    u8 s = 1;
    u8 i = 0;

    switch (opcode)
    {
        case 0b0000: // AND
            result = op1 & op2;
            setRegister(Rd, result);
            updateFlagsLogical(result, carry);
            break;

        case 0b0001: // EOR
            result = op1 ^ op2;
            setRegister(Rd, result);
            updateFlagsLogical(result, carry);
            break;

        case 0b0010: // LSL
            carry = barrelShift(op1 & 0xFF, op2, 0b00);
            setRegister(Rd, op2);
            updateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b0011: // LSR
            carry = barrelShift(op1 & 0xFF, op2, 0b01);
            setRegister(Rd, op2);
            updateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b0100: // ASR
            carry = barrelShift(op1 & 0xFF, op2, 0b10);
            setRegister(Rd, op2);
            updateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b0101: // ADC
            result = op1 + op2 + carry;
            setRegister(Rd, result);
            updateFlagsAddition(op1, op2, result);
            break;

        case 0b0110: // SBC
            result = op2 - op1 - (~carry & 0x1); // Rd - Rs - NOT C-bit
            setRegister(Rd, result);
            updateFlagsSubtraction(op2, op1, result);
            break;

        case 0b0111: // ROR
            carry = barrelShift(op1 & 0xFF, op2, 0b11);
            setRegister(Rd, op2);
            updateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b1000: // TST
            result = op1 & op2;
            updateFlagsLogical(result, carry);
            break;

        case 0b1001: // NEG
            result = 0 - op1;
            setRegister(Rd, result);
            updateFlagsSubtraction(0, op1, result);
            break;

        case 0b1010: // CMP
            result = op2 - op1;
            updateFlagsSubtraction(op2, op1, result);
            break;

        case 0b1011: // CMN
            result = op2 + op1;
            updateFlagsAddition(op1, op2, result);
            break;

        case 0b1100: // ORR
            result = op2 | op1;
            setRegister(Rd, result);
            updateFlagsLogical(result, carry);
            break;

        case 0b1101: // MUL
        {
            result = op2 * op1;
            setRegister(Rd, result);
            
            // mul uses slightly different cpsr updates than addition

            u8 new_n_flag = result & 0x80000000 ? 1 : 0; // bit 31 of result
            setConditionCodeFlag(ConditionFlag::N, new_n_flag);

            u8 new_z_flag = result == 0 ? 1 : 0;
            setConditionCodeFlag(ConditionFlag::Z, new_z_flag);

            // C destroyed
            setConditionCodeFlag(ConditionFlag::C, 0);
            break;
        }

        case 0b1110: // BIC
            result = op2 & ~op1;
            setRegister(Rd, result);
            updateFlagsLogical(result, carry);
            break;

        case 0b1111: // MVN
            result = ~op1;
            setRegister(Rd, result);
            updateFlagsLogical(result, carry);
            break;

        default:
            LOG(LogLevel::Error, "Error: Invalid thumb ALU opcode\n");
            return;
    }

    tick(n, s, i);
}

void Arm7Tdmi::hiRegisterOps(u16 instruction)
{
    u16 Rs     = util::bitseq<5, 3>(instruction);
    u16 Rd     = util::bitseq<2, 0>(instruction); 
    u16 opcode = util::bitseq<9, 8>(instruction);

    bool H1 = util::bitseq<7, 7>(instruction) == 0x1; // Hi operand flag 1
    bool H2 = util::bitseq<6, 6>(instruction) == 0x1; // Hi operand flag 2

    // access hi registers (need a 4th bit)
    if (H2) Rs |= 0b1000;
    if (H1) Rd |= 0b1000;

    u32 op1 = getRegister(Rs);
    u32 op2 = getRegister(Rd);
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
                LOG(LogLevel::Error, "Error: H1 = 0 and H2 = 0 for thumb ADD is not defined\n");
                return;
            }

            // clear bit 0 when destination is r15
            if (Rd == r15)
                op1 &= ~0x1;

            result = op1 + op2;
            setRegister(Rd, result);

            if (Rd == r15)
            {
                pipeline_full = false;

                // +1S, +1N if r15 is destination
                ++s;
                ++n;
            }
        
            break;

        case 0b01: // CMP
            if (!H1 && !H2)
            {
                LOG(LogLevel::Error, "Error: H1 = 0 and H2 = 0 for thumb CMP is not defined\n");
                return;
            }

            result = op2 - op1;
            updateFlagsSubtraction(op2, op1, result);
            break;
        case 0b10: // MOV
            if (!H1 && !H2)
            {
                LOG(LogLevel::Error, "Error: H1 = 0 and H2 = 0 for thumb MOV is not defined\n");
                return;
            }

            // clear bit 0 when destination is r15
            if (Rd == r15)
                op1 &= ~0x1;

            result = op1;
            setRegister(Rd, result);

            if (Rd == r15)
            {
                pipeline_full = false;

                // +1S, +1N if r15 is destination
                ++s;
                ++n;
            }

            break;
        case 0b11: // BX
            if (H1)
            {
                LOG(LogLevel::Error, "Error: H1 = 1 for thumb BX is not defined\n");
                return;
            }

            // swith to ARM mode if necessary
            if ((op1 & 1) == 0)
            {
                // align to word boundary
                op1 &= ~3;
                setRegister(r15, op1);
                setState(State::ARM);
            }

            else
            {
                // clear bit 0
                op1 &= ~1;
            }

            setRegister(r15, op1);

            // flush pipeline for refill
            pipeline_full = false;

            ++s;
            ++n;
        break;
    }

    // cycles:
    // 1S for ADD/MOV/CMP
    // 2S + 1N for Rd = 15 or BX
    tick(n, s, i);
}

void Arm7Tdmi::pcRelLoad(u16 instruction)
{
    u16 Rd = util::bitseq<10, 8>(instruction); 
    u16 word8 = util::bitseq<7, 0>(instruction);
    u32 base = getRegister(r15);
    base &= ~2; // clear bit 1 for word alignment

    word8 <<= 2; // assembler places #imm >> 2 in word8
    base += word8;

    setRegister(Rd, read32(base, true));
    
    // cycles: 1S + 1N + 1I
    tick(1, 1, 1);
}

void Arm7Tdmi::loadStoreRegOffset(u16 instruction)
{
    u16 Ro = util::bitseq<8, 6>(instruction); // offset register
    u16 Rb = util::bitseq<5, 3>(instruction); // base register
    u16 Rd = util::bitseq<2, 0>(instruction); // destination register

    bool load = util::bitseq<11, 11>(instruction) == 1;
    bool byte = util::bitseq<10, 10>(instruction) == 1;

    u32 base = getRegister(Rb);
    base += getRegister(Ro); // add offset to base

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    if (load)
    {
        if (byte)
            setRegister(Rd, read8(base));
        else
            setRegister(Rd, read32(base, true));
        
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        if (byte)
            write8(base, getRegister(Rd) & 0xFF);
        else
            write32(base, getRegister(Rd));

        n = 2;    
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    tick(n, s, i);
}

void Arm7Tdmi::loadStoreSignedHalfword(u16 instruction)
{
    u16 Ro = util::bitseq<8, 6>(instruction); // offset register
    u16 Rb = util::bitseq<5, 3>(instruction); // base register
    u16 Rd = util::bitseq<2, 0>(instruction); // destination register

    bool H = util::bitseq<11, 11>(instruction) == 1; // H flag
    bool S = util::bitseq<10, 10>(instruction) == 1; // sign extended flag

    u32 base = getRegister(Rb);
    base += getRegister(Ro); // add offset to base

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    // store halfword
    if (!S && !H)
    {
        write16(base, getRegister(Rd) & 0xFFFF);
        n = 2;
    }
    
    // load halfword
    else if (!S && H)
    {
        u32 value = read16(base, false);
        setRegister(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }
    
    // load sign-extended byte
    else if (S && !H)
    {
        u32 value = read8(base);
        if (value & 0x80)
            value |= 0xFFFFFF00; // bit 7 of byte is 1, so sign extend bits 31-8 of register
        setRegister(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }
    
    // load sign-extended halfword
    else
    {
        u32 value = read16(base, true);
        setRegister(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    tick(n, s, i);
}

void Arm7Tdmi::loadStoreImmediate(u16 instruction)
{
    u16 Rb      = util::bitseq<5, 3>(instruction);  // base register
    u16 Rd      = util::bitseq<2, 0>(instruction);  // destination register
    u16 offset5 = util::bitseq<10, 6>(instruction); // 5 bit immediate offset

    bool byte = util::bitseq<12, 12>(instruction) == 1;
    bool load = util::bitseq<11, 11>(instruction) == 1;
    
    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    if (!byte)
        offset5 <<= 2; // assembler places #imm >> 2 in word5 for word accesses

    u32 base = getRegister(Rb);
    base += offset5; // add offset to base

    // store word
    if (!load && !byte)
    { 
        write32(base, getRegister(Rd));
        n = 2;
    }
    
    // load word
    else if (load && !byte)
    {
        setRegister(Rd,  read32(base, true));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store byte
    else if (!load && byte)
    {
        write8(base, getRegister(Rd) & 0xFF);
        n = 2;
    }
    
    else
    { // load byte
        setRegister(Rd, read8(base));
        n = 1;
        s = 1;
        i = 1;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    tick(n, s, i);
}

void Arm7Tdmi::loadStoreHalfword(u16 instruction)
{
    u16 Rb      = util::bitseq<5, 3>(instruction);  // base register
    u16 Rd      = util::bitseq<2, 0>(instruction);  // destination register
    u16 offset5 = util::bitseq<10, 6>(instruction); // 5 bit immediate offset

    offset5 <<= 1; // assembler places #imm >> 1 in word5 to ensure halfword alignment
    bool load = util::bitseq<11, 11>(instruction) == 1;

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    u32 base = getRegister(Rb);
    base += offset5; // add offset to base

    if (load)
    {
        setRegister(Rd, read16(base, false));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        write16(base, getRegister(Rd) & 0xFFFF);
        n = 2;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    tick(n, s, i);
}

void Arm7Tdmi::spRelLoadStore(u16 instruction)
{
    u16 Rd    = util::bitseq<10, 8>(instruction); // destination register
    u16 word8 = util::bitseq<7, 0>(instruction);  // 8 bit immediate offset
    bool load = util::bitseq<11, 11>(instruction) == 1;

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = getRegister(r13); // current stack pointer is base address
    base += word8; // add offset to base

    if (load)
    {
        setRegister(Rd, read32(base, true));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        write32(base, getRegister(Rd));
        n = 2;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    tick(n, s, i);
}

void Arm7Tdmi::loadAddress(u16 instruction)
{
    u16 Rd    = util::bitseq<10, 8>(instruction);       // destination register
    u16 word8 = util::bitseq<7, 0>(instruction);        // 8 bit immediate offset
    bool sp   = util::bitseq<11, 11>(instruction) == 1; // stack pointer if true, else PC
    u32 base;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    if (sp)
    {
        base = getRegister(r13);
    }
    
    // pc
    else
    {
        base = getRegister(r15);
        base &= ~2; // force bit 1 of PC to 0
    }

    base += word8;

    setRegister(Rd, base);

    // cycles: 1S
    tick(0, 1, 0);
}

void Arm7Tdmi::addOffsetToSp(u16 instruction)
{
    u16 sword8 = util::bitseq<6, 0>(instruction); // 7 bit signed immediate value
    bool positive = util::bitseq<7, 7>(instruction) == 0; // sign bit of sword8

    sword8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = getRegister(r13); // base address at SP

    if (positive)
        base += sword8;
    else
        base -= sword8;

    setRegister(r13, base);
    
    // cycles: 1S
    tick(0, 1, 0);
}

void Arm7Tdmi::pushPop(u16 instruction)
{
    bool load = util::bitseq<11, 11>(instruction) == 1;
    bool R    = util::bitseq<8, 8>(instruction) == 1; // PC/LR bit
    u32 base  = getRegister(r13); // base address at SP

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
    //         set_register(r15, read_u32(base, false));
    //         pipeline_full = false;
    //     }

    //     else // store r15
    //     {
    //         write_u32(base, registers.r15 + 4);
    //         increment_pc();
    //     }

    //     // store Rb = Rb +/- 0x40
    //     set_register(r13, base + 0x4);

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
        setRegister(r13, base);
        
        // push registers
        for (int i = 0; i < num_registers; ++i)
        {
            write32(base, getRegister(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
            ++s;
        }

        if (R) // push LR
        {
            write32(base, getRegister(r14));
            // base -= 4; // increment stack pointer (4 bytes for word alignment)
            ++s;
        }
    }
    
    else // POP Rlist
    {
        n = 1;
        i = 1;
        for (int i = 0; i < num_registers; ++i)
        {
            setRegister(set_registers[i], read32(base, false));
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            ++s;
        }

        if (R) // pop pc
        {
            setRegister(r15, read32(base, false) & ~1); // guaruntee halfword alignment
            pipeline_full = false;
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            ++s;
            ++n;
        }

        // write base back into sp
        setRegister(r13, base);
    }

    // cycles:
    // nS + 1N + 1I (POP)
    // (n + 1)S + 2N + 1I (POP PC)
    // (n-1)S + 2N (PUSH).
    tick(n, s, i);
}

void Arm7Tdmi::multipleLoadStore(u16 instruction)
{
    u16 Rb    = util::bitseq<10, 8>(instruction); // base register
    bool load = util::bitseq<11, 11>(instruction) == 1;
    u32 base  = getRegister(Rb);

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
            setRegister(r15, read32(base, false));
            pipeline_full = false;
        }

        else // store r15
        {
            write32(base, registers.r15 + 4);
        }

        // store Rb = Rb +/- 0x40
        setRegister(Rb, base + 0x40);

        return;
    }

    if (load)
    { 
        for (int i = 0; i < num_registers; ++i)
        {
            setRegister(set_registers[i], read32(base, false));
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
            write32(base, getRegister(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
            ++s;
        }

        n = 2;
    }

    // write back address into Rb
    setRegister(Rb, base);

    // cycles:
    // nS + 1N + 1I for LDM
    // (n - 1)S + 2N for STM
    tick(n, s, i);
}

void Arm7Tdmi::conditionalBranch(u16 instruction)
{
    u16 soffset8 = util::bitseq<7, 0>(instruction); // signed 8 bit offset
    Condition condition = (Condition) util::bitseq<11, 8>(instruction);
    u32 base = getRegister(r15);
    u32 jump_address;
    if (!conditionMet(condition))
    {
        // 1S
        tick(0, 1, 0);
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

    setRegister(r15, jump_address);

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    tick(1, 2, 0);
}

void Arm7Tdmi::softwareInterruptThumb(u16 instruction)
{
    //LOG(LogLevel::Debug, "Thumb SWI: {:x}\n", (int) (instruction & 0xFF));
    last_read_bios = bios_read_state[3];
    pipeline_full = false;

    // HLE BIOS calls
    // bits 7 - 0 determine which interrupt
    //switch (instruction & 0xFF)
    //{
        //case 0x0:  SwiSoftReset();        return;
        //case 0x1:  SwiRegisterRamReset(); return;
        //case 0x5:  SwiVBlankIntrWait();   return;
        //case 0x6:  swiDivision();         return;
        //case 0x8:  SwiSqrt();             return;
        //case 0xA:  SwiArctan2();          return;
        //case 0xB:  swiCpuSet();             return;
        //case 0xF:  SwiObjAffineSet();     return;
        //case 0x10: SwiBitUnpack();        return;
        //case 0x15: swi_RLUnCompVRAM();    return
        //default:
        // std::cout << "Unknown SWI code: " << std::hex << (instruction & 0xFF) << "\n";
    //}


    // LLE BIOS calls - handle thru BIOS
    u32 old_cpsr = getRegister(cpsr);
    setMode(Mode::SVC);
    setRegister(r14, getRegister(r15) - 2);
    registers.cpsr.i = 1;
    updateSPSR(old_cpsr, false);
    setState(State::ARM);
    setRegister(r15, 0x08);
    pipeline_full = false;

    last_read_bios = bios_read_state[3];

    // cycles: 2S + 1N
    tick(1, 2, 0);
}

void Arm7Tdmi::unconditionalBranch(u16 instruction)
{
    u16 offset11 = util::bitseq<10, 0>(instruction); // signed 11 bit offset
    u32 base = getRegister(r15);
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

    setRegister(r15, jump_address);

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    tick(1, 2, 0);
}

void Arm7Tdmi::longBranchLink(u16 instruction)
{
    u32 offset = util::bitseq<10, 0>(instruction);       // long branch offset
    bool H     = util::bitseq<11, 11>(instruction) == 1; // high/low offset bit
    u32 base;

    if (H) // instruction 2
    {
        base = getRegister(r14); // LR
        offset <<= 1;
        base += offset;

        // get address of next instruction and set bit 0
        u32 next_instruction_address = getRegister(r15) - 2;
        next_instruction_address |= 0x1;

        setRegister(r15, base);
        setRegister(r14, next_instruction_address); // next instruction in link register

        // flush pipeline for refill
        pipeline_full = false;
        
        // cycles: 3S + 1N
        tick(1, 3, 0);
    }
    
    else // instruction 1
    {
        base = getRegister(r15); // PC
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

        setRegister(r14, base); // resulting address stored in LR
    }
}

