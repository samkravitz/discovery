/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: HandlerThumb.cpp
 * DATE: October 8th, 2020
 * DESCRIPTION: execution of thumb instructions
 */
#include "Arm7Tdmi.h"

void Arm7Tdmi::MoveShiftedRegister(u16 instruction)
{
    u16 Rs         = Util::bitseq<5, 3>(instruction);
    u16 Rd         = Util::bitseq<2, 0>(instruction); 
    u32 offset5    = Util::bitseq<10, 6>(instruction); // 5 bit immediate offset
    u16 shift_type = Util::bitseq<12, 11>(instruction);
    u32 op1 = GetRegister(Rs);

    // encodings of LSR #0, ASR #0, and ROR #0 should be interpreted as #LSR #2, ASR #32, and ROR #32
    if (offset5 == 0 && shift_type != 0) // shift_type == 0 is LSL
    {
        if (shift_type == 0b11) // rotate right extended
            offset5 = 0xFFFFFFFF;
        else // LSR #32 or ASR #32
            offset5 = 32;
    }

    u8 carry_out = BarrelShift(offset5, op1, shift_type);
    SetRegister(Rd, op1);
    UpdateFlagsLogical(op1, carry_out);

    // cycles: 1S
    Tick(0, 1, 0);
}

void Arm7Tdmi::AddSubtract(u16 instruction)
{
    u16 Rs         = Util::bitseq<5, 3>(instruction);
    u16 Rd         = Util::bitseq<2, 0>(instruction);
    u16 Rn_offset3 = Util::bitseq<8, 6>(instruction);
    bool immediate = Util::bitseq<10, 10>(instruction) == 1;
    bool add       = Util::bitseq<9, 9>(instruction) == 0;
    u32 op1, op2, result;

    op1 = GetRegister(Rs);

    if (immediate)
        op2 = Rn_offset3;
    else
        op2 = GetRegister(Rn_offset3);

    if (add)
    {
        result = op1 + op2;
        UpdateFlagsAddition(op1, op2, result);
    }
    
    else
    {
        result = op1 - op2;
        UpdateFlagsSubtraction(op1, op2, result);
    }

    SetRegister(Rd, result);

    // cycles: 1S
    Tick(0, 1, 0);
}

void Arm7Tdmi::MoveImmediate(u16 instruction)
{
    u16 offset8 = Util::bitseq<7, 0>(instruction);
    u16 Rd      = Util::bitseq<10, 8>(instruction);
    u16 opcode  = Util::bitseq<12, 11>(instruction);
    u32 result;
    u8 carry = GetConditionCodeFlag(ConditionFlag::C);
    u32 operand = GetRegister(Rd);

    switch (opcode)
    {
        case 0: // MOV
            result = offset8;
            SetRegister(Rd, result);
            UpdateFlagsLogical(result, carry);
            break;
        case 1: // CMP
            result = operand - offset8;
            UpdateFlagsSubtraction(operand, offset8, result);
            break;
        case 2: // ADD
            result = operand + offset8;
            SetRegister(Rd, result);
            UpdateFlagsAddition(operand, offset8, result);
            break;
        case 3: // SUB
            result = operand - offset8;
            SetRegister(Rd, result);
            UpdateFlagsSubtraction(operand, offset8, result);
            break;
    }

    // cycles: 1S
    Tick(0, 1, 0);
}

void Arm7Tdmi::AluThumb(u16 instruction)
{
    u16 Rs     = Util::bitseq<5, 3>(instruction);
    u16 Rd     = Util::bitseq<2, 0>(instruction); 
    u16 opcode = Util::bitseq<9, 6>(instruction);
    u32 op1    = GetRegister(Rs);
    u32 op2    = GetRegister(Rd);
    u8 carry   = GetConditionCodeFlag(ConditionFlag::C);
    u32 result;

    // cycles
    u8 n = 0;
    u8 s = 1;
    u8 i = 0;

    switch (opcode)
    {
        case 0b0000: // AND
            result = op1 & op2;
            SetRegister(Rd, result);
            UpdateFlagsLogical(result, carry);
            break;

        case 0b0001: // EOR
            result = op1 ^ op2;
            SetRegister(Rd, result);
            UpdateFlagsLogical(result, carry);
            break;

        case 0b0010: // LSL
            carry = BarrelShift(op1 & 0xFF, op2, 0b00);
            SetRegister(Rd, op2);
            UpdateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b0011: // LSR
            carry = BarrelShift(op1 & 0xFF, op2, 0b01);
            SetRegister(Rd, op2);
            UpdateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b0100: // ASR
            carry = BarrelShift(op1 & 0xFF, op2, 0b10);
            SetRegister(Rd, op2);
            UpdateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b0101: // ADC
            result = op1 + op2 + carry;
            SetRegister(Rd, result);
            UpdateFlagsAddition(op1, op2, result);
            break;

        case 0b0110: // SBC
            result = op2 - op1 - (~carry & 0x1); // Rd - Rs - NOT C-bit
            SetRegister(Rd, result);
            UpdateFlagsSubtraction(op2, op1, result);
            break;

        case 0b0111: // ROR
            carry = BarrelShift(op1 & 0xFF, op2, 0b11);
            SetRegister(Rd, op2);
            UpdateFlagsLogical(op2, carry);

            ++i; // 1I
            break;

        case 0b1000: // TST
            result = op1 & op2;
            UpdateFlagsLogical(result, carry);
            break;

        case 0b1001: // NEG
            result = 0 - op1;
            SetRegister(Rd, result);
            UpdateFlagsSubtraction(op2, op1, result);
            break;

        case 0b1010: // CMP
            result = op2 - op1;
            UpdateFlagsSubtraction(op2, op1, result);
            break;

        case 0b1011: // CMN
            result = op2 + op1;
            UpdateFlagsAddition(op1, op2, result);
            break;

        case 0b1100: // ORR
            result = op2 | op1;
            SetRegister(Rd, result);
            UpdateFlagsLogical(result, carry);
            break;

        case 0b1101: // MUL
            result = op2 * op1;
            SetRegister(Rd, result);
            UpdateFlagsAddition(op1, op2, result);
            break;

        case 0b1110: // BIC
            result = op2 & ~op1;
            SetRegister(Rd, result);
            UpdateFlagsLogical(result, carry);
            break;

        case 0b1111: // MVN
            result = ~op1;
            SetRegister(Rd, result);
            UpdateFlagsLogical(result, carry);
            break;

        default:
            LOG(LogLevel::Error, "Error: Invalid thumb ALU opcode\n");
            return;
    }

    Tick(n, s, i);
}

void Arm7Tdmi::HiRegisterOps(u16 instruction)
{
    u16 Rs     = Util::bitseq<5, 3>(instruction);
    u16 Rd     = Util::bitseq<2, 0>(instruction); 
    u16 opcode = Util::bitseq<9, 8>(instruction);

    bool H1 = Util::bitseq<7, 7>(instruction) == 0x1; // Hi operand flag 1
    bool H2 = Util::bitseq<6, 6>(instruction) == 0x1; // Hi operand flag 2

    // access hi registers (need a 4th bit)
    if (H2) Rs |= 0b1000;
    if (H1) Rd |= 0b1000;

    u32 op1 = GetRegister(Rs);
    u32 op2 = GetRegister(Rd);
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
            SetRegister(Rd, result);

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
            UpdateFlagsSubtraction(op2, op1, result);
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
            SetRegister(Rd, result);

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
                SetRegister(r15, op1);
                SetState(State::ARM);
            }

            else
            {
                // clear bit 0
                op1 &= ~1;
            }

            SetRegister(r15, op1);

            // flush pipeline for refill
            pipeline_full = false;

            ++s;
            ++n;
        break;
    }

    // cycles:
    // 1S for ADD/MOV/CMP
    // 2S + 1N for Rd = 15 or BX
    Tick(n, s, i);
}

void Arm7Tdmi::PcRelLoad(u16 instruction)
{
    u16 Rd = Util::bitseq<10, 8>(instruction); 
    u16 word8 = Util::bitseq<7, 0>(instruction);
    u32 base = GetRegister(r15);
    base &= ~2; // clear bit 1 for word alignment

    word8 <<= 2; // assembler places #imm >> 2 in word8
    base += word8;

    SetRegister(Rd, Read32(base, true));
    
    // cycles: 1S + 1N + 1I
    Tick(1, 1, 1);
}

void Arm7Tdmi::LoadStoreRegOffset(u16 instruction)
{
    u16 Ro = Util::bitseq<8, 6>(instruction); // offset register
    u16 Rb = Util::bitseq<5, 3>(instruction); // base register
    u16 Rd = Util::bitseq<2, 0>(instruction); // destination register

    bool load = Util::bitseq<11, 11>(instruction) == 1;
    bool byte = Util::bitseq<10, 10>(instruction) == 1;

    u32 base = GetRegister(Rb);
    base += GetRegister(Ro); // add offset to base

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    if (load)
    {
        if (byte)
            SetRegister(Rd, Read8(base));
        else
            SetRegister(Rd, Read32(base, true));
        
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        if (byte)
            Write8(base, GetRegister(Rd) & 0xFF);
        else
            Write32(base, GetRegister(Rd));

        n = 2;    
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    Tick(n, s, i);
}

void Arm7Tdmi::LoadStoreSignedHalfword(u16 instruction)
{
    u16 Ro = Util::bitseq<8, 6>(instruction); // offset register
    u16 Rb = Util::bitseq<5, 3>(instruction); // base register
    u16 Rd = Util::bitseq<2, 0>(instruction); // destination register

    bool H = Util::bitseq<11, 11>(instruction) == 1; // H flag
    bool S = Util::bitseq<10, 10>(instruction) == 1; // sign extended flag

    u32 base = GetRegister(Rb);
    base += GetRegister(Ro); // add offset to base

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    // store halfword
    if (!S && !H)
    {
        Write16(base, GetRegister(Rd) & 0xFFFF);
        n = 2;
    }
    
    // load halfword
    else if (!S && H)
    {
        u32 value = Read16(base, false);
        SetRegister(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }
    
    // load sign-extended byte
    else if (S && !H)
    {
        u32 value = Read8(base);
        if (value & 0x80)
            value |= 0xFFFFFF00; // bit 7 of byte is 1, so sign extend bits 31-8 of register
        SetRegister(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }
    
    // load sign-extended halfword
    else
    {
        u32 value = Read16(base, true);
        SetRegister(Rd, value);
        n = 1;
        s = 1;
        i = 1;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    Tick(n, s, i);
}

void Arm7Tdmi::LoadStoreImmediate(u16 instruction)
{
    u16 Rb      = Util::bitseq<5, 3>(instruction);  // base register
    u16 Rd      = Util::bitseq<2, 0>(instruction);  // destination register
    u16 offset5 = Util::bitseq<10, 6>(instruction); // 5 bit immediate offset

    bool byte = Util::bitseq<12, 12>(instruction) == 1;
    bool load = Util::bitseq<11, 11>(instruction) == 1;
    
    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    if (!byte)
        offset5 <<= 2; // assembler places #imm >> 2 in word5 for word accesses

    u32 base = GetRegister(Rb);
    base += offset5; // add offset to base

    // store word
    if (!load && !byte)
    { 
        Write32(base, GetRegister(Rd));
        n = 2;
    }
    
    // load word
    else if (load && !byte)
    {
        SetRegister(Rd,  Read32(base, true));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store byte
    else if (!load && byte)
    {
        Write8(base, GetRegister(Rd) & 0xFF);
        n = 2;
    }
    
    else
    { // load byte
        SetRegister(Rd, Read8(base));
        n = 1;
        s = 1;
        i = 1;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    Tick(n, s, i);
}

void Arm7Tdmi::LoadStoreHalfword(u16 instruction)
{
    u16 Rb      = Util::bitseq<5, 3>(instruction);  // base register
    u16 Rd      = Util::bitseq<2, 0>(instruction);  // destination register
    u16 offset5 = Util::bitseq<10, 6>(instruction); // 5 bit immediate offset

    offset5 <<= 1; // assembler places #imm >> 1 in word5 to ensure halfword alignment
    bool load = Util::bitseq<11, 11>(instruction) == 1;

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    u32 base = GetRegister(Rb);
    base += offset5; // add offset to base

    if (load)
    {
        SetRegister(Rd, Read16(base, false));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        Write16(base, GetRegister(Rd) & 0xFFFF);
        n = 2;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    Tick(n, s, i);
}

void Arm7Tdmi::SpRelLoadStore(u16 instruction)
{
    u16 Rd    = Util::bitseq<10, 8>(instruction); // destination register
    u16 word8 = Util::bitseq<7, 0>(instruction);  // 8 bit immediate offset
    bool load = Util::bitseq<11, 11>(instruction) == 1;

    // cycles
    u8 n = 0;
    u8 s = 0;
    u8 i = 0;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = GetRegister(r13); // current stack pointer is base address
    base += word8; // add offset to base

    if (load)
    {
        SetRegister(Rd, Read32(base, true));
        n = 1;
        s = 1;
        i = 1;
    }
    
    // store
    else
    {
        Write32(base, GetRegister(Rd));
        n = 2;
    }

    // cycles:
    // 1S + 1N + 1I for LDR
    // 2N for STR
    Tick(n, s, i);
}

void Arm7Tdmi::LoadAddress(u16 instruction)
{
    u16 Rd    = Util::bitseq<10, 8>(instruction);       // destination register
    u16 word8 = Util::bitseq<7, 0>(instruction);        // 8 bit immediate offset
    bool sp   = Util::bitseq<11, 11>(instruction) == 1; // stack pointer if true, else PC
    u32 base;

    word8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    if (sp)
    {
        base = GetRegister(r13);
    }
    
    // pc
    else
    {
        base = GetRegister(r15);
        base &= ~2; // force bit 1 of PC to 0
    }

    base += word8;

    SetRegister(Rd, base);

    // cycles: 1S
    Tick(0, 1, 0);
}

void Arm7Tdmi::AddOffsetToSp(u16 instruction)
{
    u16 sword8 = Util::bitseq<6, 0>(instruction); // 7 bit signed immediate value
    bool positive = Util::bitseq<7, 7>(instruction) == 0; // sign bit of sword8

    sword8 <<= 2; // assembler places #imm >> 2 in word8 to ensure word alignment

    u32 base = GetRegister(r13); // base address at SP

    if (positive)
        base += sword8;
    else
        base -= sword8;

    SetRegister(r13, base);
    
    // cycles: 1S
    Tick(0, 1, 0);
}

void Arm7Tdmi::PushPop(u16 instruction)
{
    bool load = Util::bitseq<11, 11>(instruction) == 1;
    bool R    = Util::bitseq<8, 8>(instruction) == 1; // PC/LR bit
    u32 base  = GetRegister(r13); // base address at SP

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
        SetRegister(r13, base);
        
        // push registers
        for (int i = 0; i < num_registers; ++i)
        {
            Write32(base, GetRegister(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
            ++s;
        }

        if (R) // push LR
        {
            Write32(base, GetRegister(r14));
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
            SetRegister(set_registers[i], Read32(base, false));
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            ++s;
        }

        if (R) // pop pc
        {
            SetRegister(r15, Read32(base, false) & ~1); // guaruntee halfword alignment
            pipeline_full = false;
            base += 4; // decrement stack pointer (4 bytes for word alignment)
            ++s;
            ++n;
        }

        // write base back into sp
        SetRegister(r13, base);
    }

    // cycles:
    // nS + 1N + 1I (POP)
    // (n + 1)S + 2N + 1I (POP PC)
    // (n-1)S + 2N (PUSH).
    Tick(n, s, i);
}

void Arm7Tdmi::MultipleLoadStore(u16 instruction)
{
    u16 Rb    = Util::bitseq<10, 8>(instruction); // base register
    bool load = Util::bitseq<11, 11>(instruction) == 1;
    u32 base  = GetRegister(Rb);

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
            SetRegister(r15, Read32(base, false));
            pipeline_full = false;
        }

        else // store r15
        {
            Write32(base, registers.r15 + 4);
        }

        // store Rb = Rb +/- 0x40
        SetRegister(Rb, base + 0x40);

        return;
    }

    if (load)
    { 
        for (int i = 0; i < num_registers; ++i)
        {
            SetRegister(set_registers[i], Read32(base, false));
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
            Write32(base, GetRegister(set_registers[i]));
            base += 4; // increment stack pointer (4 bytes for word alignment)
            ++s;
        }

        n = 2;
    }

    // write back address into Rb
    SetRegister(Rb, base);

    // cycles:
    // nS + 1N + 1I for LDM
    // (n - 1)S + 2N for STM
    Tick(n, s, i);
}

void Arm7Tdmi::ConditionalBranch(u16 instruction)
{
    u16 soffset8 = Util::bitseq<7, 0>(instruction); // signed 8 bit offset
    Condition condition = (Condition) Util::bitseq<11, 8>(instruction);
    u32 base = GetRegister(r15);
    u32 jump_address;
    if (!ConditionMet(condition))
    {
        // 1S
        Tick(0, 1, 0);
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

    SetRegister(r15, jump_address);

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    Tick(1, 2, 0);
}

void Arm7Tdmi::SoftwareInterruptThumb(u16 instruction)
{
    LOG(LogLevel::Debug, "Thumb SWI: {}\n", instruction & 0xFF);

    // HLE BIOS calls
    // bits 7 - 0 determine which interrupt
    // switch (instruction & 0xFF)
    // {
    //     case 0x0:
    //         swi_softReset();
    //         break;
    //     case 0x1:
    //         swi_registerRamReset();
    //         break;
    //     case 0x5:
    //         swi_VBlankIntrWait();
    //         break;
    //     case 0x6:
    //         swi_division();
    //         break;
    //     case 0x8:
    //         swi_sqrt();
    //         break;
    //     case 0xA:
    //         swi_arctan2();
    //         break;
    //     case 0xB:
    //         swi_cpuSet();
    //         break;
    //     case 0xF:
    //         swi_objAffineSet();
    //         break;
    //     case 0x10:
    //         swi_bitUnpack();
    //         break;
    //     case 0x15:
    //         //swi_RLUnCompVRAM();
    //         break;
    //     default:
    //         std::cout << "Unknown SWI code: " << std::hex << (instruction & 0xFF) << "\n";
    // }

    // LLE BIOS calls - handle thru BIOS
    u32 old_cpsr = GetRegister(cpsr);
    SetMode(Mode::SVC);
    SetRegister(r14, GetRegister(r15) - 2);
    registers.cpsr.flags.i = 1;
    UpdateSPSR(old_cpsr, false);
    SetState(State::ARM);
    SetRegister(r15, 0x08);
    pipeline_full = false;

    // cycles: 2S + 1N
    Tick(1, 2, 0);
}

void Arm7Tdmi::UnconditionalBranch(u16 instruction)
{
    u16 offset11 = Util::bitseq<10, 0>(instruction); // signed 11 bit offset
    u32 base = GetRegister(r15);
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

    SetRegister(r15, jump_address);

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    Tick(1, 2, 0);
}

void Arm7Tdmi::LongBranchLink(u16 instruction)
{
    u32 offset = Util::bitseq<10, 0>(instruction);       // long branch offset
    bool H     = Util::bitseq<11, 11>(instruction) == 1; // high/low offset bit
    u32 base;

    if (H) // instruction 2
    {
        base = GetRegister(r14); // LR
        offset <<= 1;
        base += offset;

        // get address of next instruction and set bit 0
        u32 next_instruction_address = GetRegister(r15) - 2;
        next_instruction_address |= 0x1;

        SetRegister(r15, base);
        SetRegister(r14, next_instruction_address); // next instruction in link register

        // flush pipeline for refill
        pipeline_full = false;
        
        // cycles: 3S + 1N
        Tick(1, 3, 0);
    }
    
    else // instruction 1
    {
        base = GetRegister(r15); // PC
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

        SetRegister(r14, base); // resulting address stored in LR
    }
}

