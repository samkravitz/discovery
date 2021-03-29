/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: HandlerArm.cpp
 * DATE: June 27, 2020
 * DESCRIPTION: execution of arm instructions
 */

#include "Arm7Tdmi.h"

/*
 * Copies the contents of Rn (bits 3-0) of instruction into the PC,
 * flushes the pipeline, and restarts execution from the address
 * contained in Rn. If bit 0 of Rn is 1, switch to THUMB mode
 */
void Arm7Tdmi::branchExchange(u32 instruction)
{
    u32 Rn = util::bitseq<3, 0>(instruction);

    if (Rn == r15)
    {
        LOG(LogLevel::Error, "BranchExchange: Undefined behavior: r15 as operand: 0x{x}\n", registers.r15);
        setMode(Mode::UND);
        exit(0);
        return;
    }
    
    u32 branch_address = getRegister(Rn);

    setRegister(r15, branch_address); 

    // swith to THUMB mode if necessary
    if ((branch_address & 1) == 1)
    {
        registers.r15 -= 1; // continue at Rn - 1 for thumb mode
        setState(State::THUMB);
    }

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    tick(1, 2, 0);
}

void Arm7Tdmi::branchLink(u32 instruction)
{
    bool link  = util::bitseq<24, 24>(instruction);
    u32 offset = util::bitseq<23, 0>(instruction);
    bool is_neg = offset >> 23 == 0x1;
    u32 new_address;

    offset <<= 2;

    // maintain sign by padding offset sign extension to 32 bits with 1s
    if (is_neg)
        offset |= 0b1111'1100'0000'0000'0000'0000'0000'0000;

    if (link)
    {
        // write the old PC into the link register of the current bank
        // The PC value written into r14 is adjusted to allow for the prefetch, and contains the
        // address of the instruction following the branch and link instruction
        new_address = getRegister(r15) - 4; // instruction following this BL
        new_address &= ~3; // clear bits 0-1
        setRegister(r14, new_address);
    }

    new_address = getRegister(r15) + offset;
    setRegister(r15, new_address);

    // flush pipeline for refill
    pipeline_full = false;

    // cycles: 2S + 1N
    tick(1, 2, 0);
}

 void Arm7Tdmi::dataProcessing(u32 instruction)
 {
    u32 Rd = util::bitseq<15, 12>(instruction); // destination register
    u32 Rn = util::bitseq<19, 16>(instruction); // source register
    u32 op1 = getRegister(Rn);
    u32 op2;
    u32 result;

    // cycles
    u8 n = 0;
    u8 s = 1; // 1S cycles for normal data processing
    u8 i = 0;

    bool immediate = util::bitseq<25, 25>(instruction) == 0x1;
    bool set_condition_code = util::bitseq<20, 20>(instruction) == 0x1;
    
    if (Rd == r15)
    {
        // +1 N and +1 S cycles if Rd is r15
        ++n;
        ++s;
    }
    
    u8 carry_out = 2;

    // determine op2 based on whether it's encoded as an immeidate value or register shift
    if (immediate)
    {
        op2 = util::bitseq<7, 0>(instruction);
        u32 rotate = util::bitseq<11, 8>(instruction);
        rotate *= 2; // rotate by twice the value in the rotate field

        // perform right rotation
        carry_out = barrelShift(rotate, op2, 0b11); // code for ror
    }
    
    // op2 is shifted register
    else
    { 
        u32 shift = util::bitseq<11, 4>(instruction);
        u8 shift_type = util::bitseq<6, 5>(instruction);
        u32 shift_amount;
        u32 Rm = instruction & 0b1111; // bitseq<> 3-0 
        op2 = getRegister(Rm);

        // if r15 is a shifted reg and the shift amount is contained in the register,
        // the value in r15 will be 12 bytes ahead of the instruction due to prefetch
        bool prefetch = false;
    
        // get shift amount
        if ((shift & 1) == 1) // shift amount contained in bottom byte of Rs
        { 
            u32 Rs = util::bitseq<11, 8>(instruction);
            shift_amount = getRegister(Rs) & 0xFF;

            // must add 4 bytes to r15 to account for prefetch
            if (Rn == r15 || Rm == r15 || Rs == r15)
                prefetch = true;
        }
        
        else // shift contained in immediate value in instruction
        {
            shift_amount = util::bitseq<11, 7>(instruction);

            // encodings of LSR #0, ASR #0, and ROR #0 should be interpreted as LSR #32, ASR #32, and RRX
            if (shift_amount == 0 && shift_type != 0) // shift_type == 0 is LSL
            {
                if (shift_type == 0b11) // rotate right extended
                    shift_amount = 0xFFFFFFFF;
                else // LSR #32 or ASR #32
                    shift_amount = 32;
            }
        }

        carry_out = barrelShift(shift_amount, op2, shift_type);

        // must add 4 bytes to op2 to account for prefetch
        if (prefetch)
            op2 += 4;

        ++i; // + 1I cycles with register specified shift
    }
    
    // for arithmetic operations that use a carry bit, this will either equal
    // - the carry out bit of the barrel shifter (if a register shift was applied), or
    // - the existing condition code flag from the cpsr
    u8 carry = carry_out == 2 ? getConditionCodeFlag(ConditionFlag::C) : carry_out;

    // decode opcode (bits 24-21)
    switch((DataProcessingOpcodes) util::bitseq<24, 21>(instruction))
    {
        case AND: 
            result = op1 & op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        case EOR:
            result = op1 ^ op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        case SUB:
            result = op1 - op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsSubtraction(op1, op2, result);
            break;
        case RSB:
            result = op2 - op1;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsSubtraction(op2, op1, result);
            break;
        case ADD:
            result = op1 + op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsAddition(op1, op2, result);
            break;
        case ADC:
            result = op1 + op2 + getConditionCodeFlag(ConditionFlag::C);
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsAddition(op1, op2, result);
            break;
        case SBC:
            result = op1 - op2 + getConditionCodeFlag(ConditionFlag::C) - 1;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsSubtraction(op1, op2, result);
            break;
        case RSC:
            result = op2 - op1 + getConditionCodeFlag(ConditionFlag::C) - 1;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsSubtraction(op2, op1, result);
            break;
        case TST:
            result = op1 & op2;
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        case TEQ:
            result = op1 ^ op2;
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        case CMP:
            result = op1 - op2;
            if (set_condition_code) updateFlagsSubtraction(op1, op2, result);
            break;
        case CMN:
            result = op1 + op2;
            if (set_condition_code) updateFlagsAddition(op1, op2, result);
            break;
        case ORR:
            result = op1 | op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        case MOV:
            result = op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        case BIC:
            result = op1 & ~op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        case MVN:
            result = ~op2;
            setRegister(Rd, result);
            if (set_condition_code) updateFlagsLogical(result, carry);
            break;
        default:
            LOG(LogLevel::Error, "Unrecognized data processing opcode: {}\n", util::bitseq<24, 21>(instruction));
            break;
    }

    // if writing new value to PC, don't increment PC
    if (Rd == r15)
    {
        pipeline_full = false;
        // if S bit is set, move SPSR into CPSR
        if (set_condition_code)
            setRegister(cpsr, getRegister(spsr));
    }

    // cycles: (1+p)S + rI + pN
    // Whereas r=1 if I=0 and R=1 (ie. shift by register); otherwise r=0. And p=1 if Rd=R15; otherwise p=0.
    tick(n, s, i); 
}

void Arm7Tdmi::multiply(u32 instruction)
{
    // assign registers
    u32 Rm = util::bitseq<3, 0>(instruction);   // first operand
    u32 Rs = util::bitseq<11, 8>(instruction);  // source register
    u32 Rn = util::bitseq<15, 12>(instruction); // second operand
    u32 Rd = util::bitseq<19, 16>(instruction); // destination register
    bool accumulate = util::bitseq<21, 21>(instruction) == 1;    
    bool set_condition_code_flags = util::bitseq<20, 20>(instruction) == 1;

    // Conflicting sources about this
    // if (Rd == Rm) {
    //     std::cout << "Rd must not be the same as Rm" << std::endl;
    //     return;
    // }
    
    if (Rd == r15 || Rm == r15)
    {
        LOG(LogLevel::Error, "Multiply: Register 15 may not be used as destination nor operand register\n");
        return;
    }

    u8 m; // # of m cycles
    
    u32 op1 = getRegister(Rm);
    u32 op2 = getRegister(Rs);
    u32 result = op1 * op2;

    // determine how many m cycles
    if ((op2 >> 8 == 0b111111111111111111111111) ||  (op2 >> 8 == 0))
        m = 1;
    if ((op2 >> 16 == 0b1111111111111111)        ||  (op2 >> 16 == 0))
        m = 2;
    if ((op2 >> 24 == 0b11111111)                ||  (op2 >> 24 == 0))
        m = 3;
    else
        m = 4;
    
    if (accumulate)
    {
        result += getRegister(Rn); // multiply-accumulate form gives Rd:=Rm*Rs+Rn
        ++m; // for MLA (m + 1) I cycles
    }

    setRegister(Rd, result);

    if (set_condition_code_flags)
    {
        u8 new_n_flag = result & 0x80000000 ? 1 : 0; // bit 31 of result
        setConditionCodeFlag(ConditionFlag::N, new_n_flag);

        u8 new_z_flag = result == 0 ? 1 : 0;
        setConditionCodeFlag(ConditionFlag::Z, new_z_flag);

        setConditionCodeFlag(ConditionFlag::C, 1); // C is set to a meaningless value
    }

    // cycles: 1S, mI
    tick(0, 1, m);
}

void Arm7Tdmi::multiplyLong(u32 instruction)
{
    u32 RdHi                = util::bitseq<19, 16>(instruction);
    u32 RdLo                = util::bitseq<15, 12>(instruction);
    u32 Rs                  = util::bitseq<11, 8>(instruction);
    u32 Rm                  = util::bitseq<3, 0>(instruction);
    bool set_condition_code = util::bitseq<20, 20>(instruction) == 1;
    bool accumulate         = util::bitseq<21, 21>(instruction) == 1;
    bool sign               = util::bitseq<22, 22>(instruction) == 1;

    if (RdHi == r15 || RdLo == r15 || Rm == r15 || Rs == r15)
    {
        LOG(LogLevel::Error, "Multiply: Register 15 may not be used as destination nor operand register\n");
        return;
    }

    // RdHi, RdLo, and Rm must all specify different registers
    if (RdHi == RdLo || RdHi == Rm || RdLo == Rm)
    {
        LOG(LogLevel::Error, "Multiply: RdHi, RdLo, and Rm must all specify different registers\n");
        return;
    }

    u8 m; // # of m cycles

    // signed multiply long
    if (sign)
    {
        s64 op1 = (s32) getRegister(Rm);
        s64 op2 = (s32) getRegister(Rs);
        s64 result, temp;
        result = op1 * op2; // 64 bit result

        // Add contents of RdHi, RdLo to result
        if (accumulate)
        {
            u64 acc = getRegister(RdHi);
            acc <<= 16;
            acc <<= 16;
            acc |= getRegister(RdLo);
            result += (s64) acc; // C++ casting...
            
            // +1 m cycles for accumulate
            m++;
        }

        temp = result;

        s32 lo = result & 0xFFFFFFFF; // lower 32 bits of result
        result >>= 16;
        result >>= 16;
        s32 hi = result;

        result = temp;

        setRegister(RdHi, hi);
        setRegister(RdLo, lo);

        if (set_condition_code)
        {
            u8 new_n_flag = result & 0x8000000000000000 ? 1 : 0; // bit 63 of result
            setConditionCodeFlag(ConditionFlag::N, new_n_flag);

            u8 new_z_flag = result == 0 ? 1 : 0;
            setConditionCodeFlag(ConditionFlag::Z, new_z_flag);

            // C destroyed
            setConditionCodeFlag(ConditionFlag::C, 0);
        }

        // determine how many m cycles
        if ((op2 >> 8 == 0b111111111111111111111111) ||  (op2 >> 8 == 0))
            m = 1;
        if ((op2 >> 16 == 0b1111111111111111)        ||  (op2 >> 16 == 0))
            m = 2;
        if ((op2 >> 24 == 0b11111111)                ||  (op2 >> 24 == 0))
            m = 3;
        else
            m = 4;
    }

    // unsigned multiply long
    else
    {
        u64 op1 = getRegister(Rm);
        u64 op2 = getRegister(Rs);
        u64 result, temp;
        result = op1 * op2; // 64 bit result

        // Add contents of RdHi, RdLo to result
        if (accumulate)
        {
            u64 acc = (u32) getRegister(RdHi);
            acc <<= 16;
            acc <<= 16;
            acc |= (u32) getRegister(RdLo);
            result += acc;

            // +1 m cycles for accumulate
            m++;
        }

        temp = result;

        u32 lo = result & 0xFFFFFFFF; // lower 32 bits of result
        result >>= 16;
        result >>= 16;
        u32 hi = result;

        result = temp;

        setRegister(RdHi, hi);
        setRegister(RdLo, lo);

        if (set_condition_code)
        {
            u8 new_n_flag = result & 0x8000000000000000 ? 1 : 0; // bit 63 of result
            setConditionCodeFlag(ConditionFlag::N, new_n_flag);

            u8 new_z_flag = result == 0 ? 1 : 0;
            setConditionCodeFlag(ConditionFlag::Z, new_z_flag);

            // C destroyed
            setConditionCodeFlag(ConditionFlag::C, 0);
        }

        // determine how many m cycles
        if (op2 >> 8 == 0)
            m = 1;
        if (op2 >> 16 == 0)
            m = 2;
        if (op2 >> 24 == 0)
            m = 3;
        else
            m = 4;
    }

    // cycles: 1S, mI
    tick(0, 1, m);
}

// allow access to CPSR and SPSR registers
 void Arm7Tdmi::psrTransfer(u32 instruction)
 {
    bool use_spsr = util::bitseq<22, 22>(instruction) == 1;
    u32 opcode    = util::bitseq<21, 21>(instruction);

    // MRS (transfer PSR contents to register)
    if (opcode == 0)
    {
        u32 Rd = util::bitseq<15, 12>(instruction);
        if (Rd == r15) 
        {
            LOG(LogLevel::Error, "Can't use r15 as an MRS destination register\n");
            return;
        }

        if (use_spsr)
            setRegister(Rd, getRegister(spsr)); // Rd <- spsr_<mode>
        else
            setRegister(Rd, getRegister(cpsr)); // Rd <- cpsr
    }

    // MSR (transfer register contents to PSR)
    else
    {
        bool immediate  = util::bitseq<25, 25>(instruction) == 1;
        bool flags_only = util::bitseq<16, 16>(instruction) == 0;
        u32 new_value;

        // rotate on immediate value 
        if (immediate)
        { 
            new_value  = util::bitseq<7, 0>(instruction);
            u32 rotate = util::bitseq<11, 8>(instruction);
            rotate *= 2; // rotate by twice the value in the rotate field

            // perform right rotation
            barrelShift(rotate, new_value, 0b11); // code for ROR
        }
        
        // use value in register
        else
        {
            u32 Rm = util::bitseq<3, 0>(instruction);
            if (Rm == r15)
            {
                LOG(LogLevel::Error, "Can't use r15 as an MSR source register\n");
                return;
            }

            new_value = getRegister(Rm);
        }

        if (use_spsr)
            updateSPSR(new_value, flags_only);
        else
            updateCPSR(new_value, flags_only);
    }

    // cycles: 1S
    tick(0, 1, 0);
}

// store or load single value to/from memory
void Arm7Tdmi::singleDataTransfer(u32 instruction)
{
    bool immediate  = util::bitseq<25, 25>(instruction) == 0;
    bool pre_index  = util::bitseq<24, 24>(instruction) == 1; // bit 24 set = pre index, bit 24 0 = post index
    bool up         = util::bitseq<23, 23>(instruction) == 1; // bit 23 set = up, bit 23 0 = down
    bool byte       = util::bitseq<22, 22>(instruction) == 1; // bit 22 set = byte, bit 23 0 = word
    bool write_back = util::bitseq<21, 21>(instruction) == 1; // bit 21 set = write address into base, bit 21 0 = no write back
    bool load       = util::bitseq<20, 20>(instruction) == 1; // bit 20 set = load, bit 20 0 = store
    u32 Rn          = util::bitseq<19, 16>(instruction);
    u32 Rd          = util::bitseq<15, 12>(instruction);
    u32 offset_encoding = util::bitseq<11, 0>(instruction);
    u32 offset; // the actual amount to offset

    // cycles
    u8 n = 0;
    u8 i = 0;
    u8 s = 0;

    if (immediate)
        offset = offset_encoding;

    // op2 is a shifted register
    else {
        u32 shift_amount = util::bitseq<11, 7>(instruction);
        u32 offset_register = util::bitseq<3, 0>(instruction);

        if (offset_register == r15)
        {
            LOG(LogLevel::Error, "r15 may not be used as the offset register of SDT\n");
            return;
        }

        u8 shift_type = util::bitseq<6, 5>(instruction);
        offset = getRegister(offset_register);

        // encodings of LSR #0, ASR #0, and ROR #0 should be interpreted as LSR #32, ASR #32, and RRX
        if (shift_amount == 0 && shift_type != 0) // shift_type == 0 is LSL
        {
            if (shift_type == 0b11) // rotate right extended
                shift_amount = 0xFFFFFFFF;
            else // LSR #32 or ASR #32
                shift_amount = 32;
        }

        barrelShift(shift_amount, offset, shift_type); // offset will be modified to contain result of shifted register
    }

    u32 base = getRegister(Rn);

    // offset modification before transfer
    if (pre_index)
    {
        if (up)
            base += offset; // offset is added to base
        else
            base -= offset; // offset is subtracted from base
    }

    // transfer
    if (load) // load from memory to register
    { 
        if (byte) // load one byte from memory, sign extend 0s
        {
            u32 value = 0;
            value |= read8(base);
            setRegister(Rd, value);
        }
        
        // load one word from memory
        else
            setRegister(Rd, read32(base, true));

        // normal loads instructions take 1S + 1N + 1I
        ++s;
        ++i;
        ++n;

        // LDR PC takes an additional 1S + 1N cycles
        if (Rd == r15)
        {
            ++s;
            ++n;
            pipeline_full = false;
        }

    }
    
    // store from register to memory
    else
    {
        u32 value = getRegister(Rd);

        // if Rd is r15, the stored address will be the address of the current instruction plus 12
        if (Rd == r15)
            value += 4;

        if (byte) // store one byte to memory
            write8(base, value & 0xFF);  // lowest byte in register
        else // store one word into memory
            write32(base, value);

        // stores take 2N cycles to execute
        n = 2;
    }

    // offset modification after transfer
    if (!pre_index)
    {
        if (up) base += offset; // offset is added to base
        else    base -= offset; // offset is subtracted from base
    }

    if ((write_back || !pre_index) && (!load || Rd != Rn))
        setRegister(Rn, base);

    // cycles: LDR: 1S + 1N + 1I. LDR PC: 2S + 2N + 1I. STR: 2N
    tick(n, s, i);
}

// transfer halfword and signed data
void Arm7Tdmi::halfwordDataTransfer(u32 instruction)
{
    bool pre_index  = util::bitseq<24, 24>(instruction) == 1; // bit 24 set = pre index, bit 24 0 = post index
    bool up         = util::bitseq<23, 23>(instruction) == 1; // bit 23 set = up, bit 23 0 = down
    bool immediate  = util::bitseq<22, 22>(instruction) == 1;
    bool write_back = util::bitseq<21, 21>(instruction) == 1; // bit 21 set = write address into base, bit 21 0 = no write back
    bool load       = util::bitseq<20, 20>(instruction) == 1; // bit 20 set = load, bit 20 0 = store
    u32 Rn          = util::bitseq<19, 16>(instruction);      // base register
    u32 Rd          = util::bitseq<15, 12>(instruction);      // src/dest register
    u32 Rm          = util::bitseq<3, 0>(instruction);        // offset register
    u32 offset;
    u32 base = getRegister(Rn);

    // cycles
    u8 n = 0;
    u8 i = 0;
    u8 s = 0;

    if (Rm == r15)
    {
        LOG(LogLevel::Error, "r15 cannot be used as offset register for HDT\n");
        return;
    }

    if (immediate)
    {
        u32 high_nibble = util::bitseq<11, 8>(instruction);
        u32 low_nibble  = util::bitseq<3, 0>(instruction);
        offset = (high_nibble << 4) | low_nibble;
    }
    
    else
    {
        offset = getRegister(Rm);
    }

    // offset modification before transfer
    if (pre_index)
    {
        if (up) base += offset; // offset is added to base
        else base -= offset; // offset is subtracted from base
    }

    // transfer
    switch (util::bitseq<6, 5>(instruction)) // SH bitseq<>
    {
        case 0b01: // unsigned halfwords
            if (load)
                setRegister(Rd, read16(base, false));
            else
                write16(base, getRegister(Rd) & 0xFFFF);
            break;
        
        case 0b10: // signed byte
            if (load)
            {
                u32 value = (u32) read8(base);
                if (value & 0x80)
                value |= ~0b11111111; // bit 7 of byte is 1, so sign extend bits 31-8 of register
                setRegister(Rd, value);
            }
            
            else
            {
                LOG(LogLevel::Error, "Cannot store a signed byte in HDT\n");
                return;
            }
            break;
        
        case 0b11: // signed halfwords
            if (load)
            {
                u32 value = read16(base, true);
                if (value & 0x8000) value |= ~0b1111111111111111; // bit 15 of byte is 1, so sign extend bits 31-16 of register
                setRegister(Rd, value);
            }
            
            else
            {
                LOG(LogLevel::Error, "Cannot store a signed byte in HDT\n");
                return;
            }
            break;
        
        default:
            LOG(LogLevel::Error, "SH bits are 00! SWP instruction was decoded as HDT!\n");
            return;
    }

    if (!pre_index)
    {
        if (up)
            base += offset; // offset is added to base
        else
            base -= offset; // offset is subtracted from base
    }

    if ((write_back || !pre_index) && (!load || Rd != Rn))
        setRegister(Rn, base);

    // calculate cycles
    if (load)
    {
        if (Rd == r15)
        {
            ++s;
            ++n;
        }

        ++s;
        ++n;
        ++i;
    }
    
    else
    {
        n = 2;
    }

    // cycles: LDR: 1S + 1N + 1I. LDR PC: 2S + 2N + 1I. STR: 2N
    tick(n, s, i);
}

void Arm7Tdmi::blockDataTransfer(u32 instruction)
{
    bool pre_index    = util::bitseq<24, 24>(instruction) == 1; // bit 24 set = pre index, bit 24 0 = post index
    bool up           = util::bitseq<23, 23>(instruction) == 1; // bit 23 set = up, bit 23 0 = down
    bool load_psr     = util::bitseq<22, 22>(instruction) == 1; // bit 22 set = load PSR or force user mode
    bool write_back   = util::bitseq<21, 21>(instruction) == 1; // bit 21 set = write address into base, bit 21 0 = no write back
    bool load         = util::bitseq<20, 20>(instruction) == 1; // bit 20 set = load, bit 20 0 = store
    u32 Rb            = util::bitseq<19, 16>(instruction);      // base register
    u32 register_list = util::bitseq<15, 0>(instruction);
    u32 base          = getRegister(Rb);
    int num_registers = 0; // number of set bitseq<> in the register list, should be between 0-16
    int set_registers[16];
    Mode temp_mode = getMode();
    bool Rb_in_Rlist = false;

    bool r15_in_register_list = ((register_list >> 15) & 0x1) == 1;

    // cycles
    u8 n = 0;
    u8 i = 0;
    u8 s = 0;

    if (Rb == r15)
    {
        LOG(LogLevel::Error, "r15 cannot be used as base register in BDT!\n");
        return;
    }

    // edge case - empty Rlist
    if (register_list == 0)
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
        if (up)
            setRegister(Rb, base + 0x40);
        else
            setRegister(Rb, base - 0x40);

        return;
    }

    // get registers set in list
    // also determine if register list contains Rb
    for (int i = 0; i < 16; ++i)
    {
        if (register_list >> i & 1)
        {
            set_registers[num_registers] = i;
            num_registers++;

            // Rb in Rlist
            if (i == Rb)
                Rb_in_Rlist = true;
        }
    }

    u32 old_base = up ? base : base - num_registers * 4; // lowest address reached

    bool Rb_first_in_list = set_registers[0] == Rb;

    // force use user bank registers
    if (load_psr)
        setMode(Mode::USR);

    if (load) // load from memory
    { 
        ++n;
        ++i;
        if (Rb_in_Rlist)
            write_back = false;

        if (up) // addresses increment
        {
            for (int i = 0; i < num_registers; ++i)
            {
                if (pre_index) // pre increment
                    base += 4;

                setRegister(set_registers[i], read32(base, false));
                if (set_registers[i] == r15) // loading into r15
                {
                    pipeline_full = false; 
                    // +1 S, +1 N cycles for LDM PC
                    ++s;
                    ++n;
                }

                if (!pre_index) // post increment
                    base += 4;
                // +1 S cycles for each word transferred
                ++s;
            }
        }

        else // addresses decrement
        {
            for (int i = num_registers - 1; i >= 0; --i)
            {
                if (pre_index) // pre decrement
                    base -= 4;

                setRegister(set_registers[i], read32(base, false));
                if (set_registers[i] == r15) // loading into r15
                {
                    pipeline_full = false; 
                    // +1 S, +1 N cycles for LDM PC
                    ++s;
                    ++n;
                }

                if (!pre_index) // post decrement
                    base -= 4;
                
                // +1 S cycles for each word transferred
                ++s;
            }
        }
    }

    else // store to memory
    {
        n = 2; 
        if (up) // addresses increment
        {
            for (int i = 0; i < num_registers; ++i)
            {
                if (pre_index) // pre increment
                    base += 4;
                    
                u32 value = getRegister(set_registers[i]);
                // if Rd is r15, the stored address will be the address of the current instruction plus 12
                if (set_registers[i] == r15)
                    value += 4;
                write32(base, value);

                if (!pre_index) // post increment
                    base += 4;
                
                // +1 S cycles for each word transferred
                ++s;
            }
        }

        else // addresses decrement
        {
            for (int i = num_registers - 1; i >= 0; --i)
            {
                if (pre_index) // pre decrement
                    base -= 4;
                    
                u32 value = getRegister(set_registers[i]);
                // if Rd is r15, the stored address will be the address of the current instruction plus 12
                if (set_registers[i] == r15)
                    value += 4;
                write32(base, value);

                if (!pre_index) // post decrement
                    base -= 4;
                
                // +1 S cycles for each word transferred
                ++s;
            }
        }
    }
    
    if (write_back)
    {
        // edge case - Rb included in Rlist
        // if (Rb_in_Rlist)
        // {
        //     // store OLD base if Rb is FIRST entry in Rlist
        //     if (Rb_first_in_list)
        //         set_register(Rb, old_base);
        
        //     // otherwise store NEW base
        //     else
        //         set_register(Rb, base);
                       
        //}

        //else
        setRegister(Rb, base); // write back final address if LDM or Rb is NOT in Rlist 
    }

    if (load_psr)
        setMode(temp_mode); // restore cpu state
    
    // cycles:
    // For normal LDM, nS + 1N + 1I.
    // For LDM PC, (n+1)S+2N+1I. For STM (n-1)S+2N. Where n is the number of words transferred.
    tick(n, s, i);
}

void Arm7Tdmi::singleDataSwap(u32 instruction)
{
    bool byte = util::bitseq<22, 22>(instruction);
    u32 Rn    = util::bitseq<19, 16>(instruction); // base register
    u32 Rd    = util::bitseq<15, 12>(instruction); // destination register
    u32 Rm    = util::bitseq<3, 0>(instruction);   // source register

    if (Rn == r15 || Rd == r15 || Rm == r15)
    {
        LOG(LogLevel::Error, "r15 can't be used as an operand in SWP!\n");
        return;
    }

    u32 swap_address = getRegister(Rn);

    // swap a byte
    if (byte)
    {
        u8 temp = read8(swap_address);
        u8 source = getRegister(Rm) & 0xFF; // bottom byte of source register
        write8(swap_address, source);
        setRegister(Rd, temp);
    }
    
    // swap a word
    else
    {
        u32 temp = read32(swap_address, true);
        u32 source = getRegister(Rm);
        write32(swap_address, source);
        setRegister(Rd, temp);
    }

    // cycles: 1S + 2N + 1I
    tick(2, 1, 1);
}

void Arm7Tdmi::softwareInterruptArm(u32 instruction)
{
    //LOG(LogLevel::Debug, "ARM SWI: {}\n", instruction >> 16 & 0xFF);

    // LLE BIOS calls - handle thru BIOS
    u32 old_cpsr = getRegister(cpsr);
    setMode(Mode::SVC);
    setRegister(r14, getRegister(r15) - 4);
    registers.cpsr.i = 1;
    updateSPSR(old_cpsr, false); // move up
    setRegister(r15, 0x08);
    pipeline_full = false;

    last_read_bios = bios_read_state[3];

    // bits 23 - 16 determine which interrupt
    // switch (instruction >> 16 & 0xFF)
    // {
    //    case 0x0:
    //         swi_softReset();
    //         break;
    //     case 0x1:
    //         swi_registerRamReset();
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
    //         swi_cpuset();
    //         break;
    //     case 0xF:
    //         swi_objAffineset();
    //         break;
    //     case 0x10:
    //         swi_bitUnpack();
    //         break;
        
    //     default:
    //         std::cout << "Unknown SWI code: " << std::hex << (instruction >> 16 & 0xFF) << "\n";
    // }

    // cycles: 2S + 1N
    tick(1, 2, 0);
}