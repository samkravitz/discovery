/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Arm7.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of arm7tdmi functions
 */
#include <iostream>
#include <iomanip>
#include <cassert>

#include "Arm7.h"
#include "IRQ.h"

extern IRQ *irq;

// uncomment this if running tests
//#define TEST

int PRINT = 0;

Arm7::Arm7(Memory *mem) : mem(mem)
{
    registers = {0};               // zero out registers
    registers.r15 = 0x8000000;     // starting address of gamepak flash rom

    registers.r13     = 0x3007F00; // starting address of user stack
    registers.r13_svc = 0x3007FE0; // starting address of swi stack
    registers.r13_irq = 0x3007FA0; // starting address of interrupt stack

    setMode(Mode::SVC);
    setState(State::ARM);

    // initialize cpsr
    registers.cpsr.f = 1;
    registers.cpsr.i = 1;

    pipeline_full = false;
    cycles = 0;
    in_interrupt  = false;

    last_read_bios = bios_read_state[0];
    
    // different initialization for the testing environment
    #ifdef TEST
    registers.r15 = 0;
    set_state(USR);
    mem = new Memory();
    #endif
}

Mode Arm7::getMode()
{
    switch (registers.cpsr.mode)
    {
        case 0b10000: return Mode::USR;
        case 0b10001: return Mode::FIQ;
        case 0b10010: return Mode::IRQ;
        case 0b10011: return Mode::SVC;
        case 0b10111: return Mode::ABT;
        case 0b11111: return Mode::SYS;
        case 0b11011: return Mode::UND;
        default:
            assert(!"Error: unrecognized mode in Arm7::getMode");
            return (Mode) 0;
    } 
}

void Arm7::setMode(Mode mode)
{
    switch (mode)
    {
        case Mode::USR: registers.cpsr.mode = 0b10000; break;
        case Mode::FIQ: registers.cpsr.mode = 0b10001; break;
        case Mode::IRQ: registers.cpsr.mode = 0b10010; break;
        case Mode::SVC: registers.cpsr.mode = 0b10011; break;
        case Mode::ABT: registers.cpsr.mode = 0b10111; break;
        case Mode::SYS: registers.cpsr.mode = 0b11111; break;
        case Mode::UND: registers.cpsr.mode = 0b11011; break;
        default:
            assert(!"Error: unrecognized mode in Arm7::setMode");
    }
}

u8 Arm7::getConditionCodeFlag(ConditionFlag flag)
{
    switch (flag)
    {
        case ConditionFlag::N: return registers.cpsr.n; 
        case ConditionFlag::Z: return registers.cpsr.z;
        case ConditionFlag::C: return registers.cpsr.c;
        case ConditionFlag::V: return registers.cpsr.v;
        default:
            assert(!"Error: unrecognized flag in Arm7::getFlag");
            return -1;
    }
}

void Arm7::setConditionCodeFlag(ConditionFlag flag, u8 bit)
{
    // bit can only be 0 or 1
    if (bit > 1)
    {
        std::cerr << "Error: set bit must be 0 or 1, it is: " << bit << "\n";
        return;
    }

    switch (flag)
    {
        case ConditionFlag::N: registers.cpsr.n = bit; break;
        case ConditionFlag::Z: registers.cpsr.z = bit; break;
        case ConditionFlag::C: registers.cpsr.c = bit; break;
        case ConditionFlag::V: registers.cpsr.v = bit; break;
        default:
            assert(!"Error: unrecognized flag in Arm7::setConditionCodeFlag");
    }
}

// determine if the condition field of an instruction is true, given the state of the CPSR
bool Arm7::conditionMet(Condition condition)
{
    switch (condition)
    {
        case Condition::EQ: return getConditionCodeFlag(ConditionFlag::Z);  // Z set
        case Condition::NE: return !getConditionCodeFlag(ConditionFlag::Z); // Z clear
        case Condition::CS: return getConditionCodeFlag(ConditionFlag::C);  // C set
        case Condition::CC: return !getConditionCodeFlag(ConditionFlag::C); // C clear
        case Condition::MI: return getConditionCodeFlag(ConditionFlag::N);  // N set
        case Condition::PL: return !getConditionCodeFlag(ConditionFlag::N); // N Clear
        case Condition::VS: return getConditionCodeFlag(ConditionFlag::V);  // V set
        case Condition::VC: return !getConditionCodeFlag(ConditionFlag::V); // V clear
        case Condition::HI: return getConditionCodeFlag(ConditionFlag::C) && !getConditionCodeFlag(ConditionFlag::Z); // C set and Z clear
        case Condition::LS: return !getConditionCodeFlag(ConditionFlag::C) || getConditionCodeFlag(ConditionFlag::Z); // C clear or Z set
        case Condition::GE: return getConditionCodeFlag(ConditionFlag::N) == getConditionCodeFlag(ConditionFlag::V);  // N equals V
        case Condition::LT: return getConditionCodeFlag(ConditionFlag::N) != getConditionCodeFlag(ConditionFlag::V);  // N not equal V
        case Condition::GT: return !getConditionCodeFlag(ConditionFlag::Z) && (getConditionCodeFlag(ConditionFlag::N) == getConditionCodeFlag(ConditionFlag::V)); // Z clear AND (N equals V)
        case Condition::LE: return getConditionCodeFlag(ConditionFlag::Z) || (getConditionCodeFlag(ConditionFlag::N) != getConditionCodeFlag(ConditionFlag::V));  // Z set OR (N not equal to V)
        case Condition::AL: return true; // always
        default: // should never happen
            assert(!"Error: unrecognized condition in Arm7::conditionMet");
            return false;
    }
}

void Arm7::fetch()
{
    if (!pipeline_full)
    {
        // fill pipeline
        switch (getState())
        {
            case State::ARM:
                pipeline[0] = { read32(registers.r15), registers.r15 }; registers.r15 += 4;
                pipeline[1] = { read32(registers.r15), registers.r15 }; registers.r15 += 4;
                pipeline[2] = { read32(registers.r15), registers.r15 };
                break;
            case State::THUMB:
                pipeline[0] = { read16(registers.r15), registers.r15 }; registers.r15 += 2;
                pipeline[1] = { read16(registers.r15), registers.r15 }; registers.r15 += 2;
                pipeline[2] = { read16(registers.r15), registers.r15 };
                break;
        }

        pipeline_full = true;
        return;
    }

    switch (getState())
    {
        case State::ARM:
            pipeline[2] = { read32(registers.r15), registers.r15 };
            break;
        case State::THUMB:
            pipeline[2] = { read16(registers.r15), registers.r15 };
            break;
    }
}

void Arm7::decode() { }

int Arm7::execute(u32 instruction)
{
    cycles = 0;

    if (PRINT) {
    std::cout << "Executing: " << std::hex << instruction << "\n";
    if (instruction == 0)
        exit(5);
    }

    // std::cout << std::hex << registers.r15 << "\n";
    
    switch (getState())
    {
        case State::ARM:
            if (!conditionMet((Condition) util::bitseq<31, 28>(instruction)))
            {
                incrementPC();
                tick(0, 0, 1); // 1I
                return cycles;
            }
            
            switch(util::getInstructionFormat(instruction))
            {
                case ArmInstruction::BEX:  branchExchange(instruction);       break;
                case ArmInstruction::B:    branchLink(instruction);           break;
                case ArmInstruction::DP:   dataProcessing(instruction);       break;
                case ArmInstruction::MUL:  multiply(instruction);             break;
                case ArmInstruction::MULL: multiplyLong(instruction);         break;
                case ArmInstruction::PSR:  psrTransfer(instruction);          break;
                case ArmInstruction::SDT:  singleDataTransfer(instruction);   break;
                case ArmInstruction::HDT:  halfwordDataTransfer(instruction); break;
                case ArmInstruction::BDT:  blockDataTransfer(instruction);    break;
                case ArmInstruction::SWP:  singleDataSwap(instruction);       break;
                case ArmInstruction::INT:  softwareInterruptArm(instruction); break;
                default:
                    log(LogLevel::Error, "Cannot execute instruction {:x}, pc {:x}\n", (int) instruction, (int) registers.r15);
                    registers.r15 &= ~0x3;
            }
            break;

        case State::THUMB:
            u16 instr = static_cast<u16>(instruction);
            switch(util::getInstructionFormat(instr))
            {
                case ThumbInstruction::MSR:    moveShiftedRegister(instr);     break;
                case ThumbInstruction::ADDSUB: addSubtract(instr);             break;
                case ThumbInstruction::IMM:    moveImmediate(instr);           break;
                case ThumbInstruction::ALU:    aluThumb(instr);                break;
                case ThumbInstruction::HI:     hiRegisterOps(instr);           break;
                case ThumbInstruction::PC:     pcRelLoad(instr);               break;
                case ThumbInstruction::MOV:    loadStoreRegOffset(instr);      break;
                case ThumbInstruction::MOVS:   loadStoreSignedHalfword(instr); break;
                case ThumbInstruction::MOVI:   loadStoreImmediate(instr);      break;
                case ThumbInstruction::MOVH:   loadStoreHalfword(instr);       break;
                case ThumbInstruction::SP:     spRelLoadStore(instr);          break;
                case ThumbInstruction::LDA:    loadAddress(instr);             break;
                case ThumbInstruction::ADDSP:  addOffsetToSp(instr);           break;
                case ThumbInstruction::POP:    pushPop(instr);                 break;
                case ThumbInstruction::MOVM:   multipleLoadStore(instr);       break;
                case ThumbInstruction::B:      conditionalBranch(instr);       break;
                case ThumbInstruction::SWI:    softwareInterruptThumb(instr);  break;
                case ThumbInstruction::BAL:    unconditionalBranch(instr);     break;
                case ThumbInstruction::BL:     longBranchLink(instr);          break;
                default:
                    std::cerr << "Cannot execute thumb instruction: " << (u16) instruction << " " << std::hex << registers.r15 << "\n";
                    registers.r15 &= ~0x1;
            }
            break;
    }

    // increment pc if there was no branch
    if (pipeline_full)
        incrementPC();

    if (PRINT)
        print();

    return cycles;
}

u32 Arm7::getRegister(u32 reg)
{
    switch (reg)
    {
        case r0: return registers.r0;
        case r1: return registers.r1;
        case r2: return registers.r2;
        case r3: return registers.r3;
        case r4: return registers.r4;
        case r5: return registers.r5;
        case r6: return registers.r6;
        case r7: return registers.r7;

        case r8:
            switch (getMode())
            {
                case Mode::FIQ: return registers.r8_fiq;
                default: return registers.r8;
            }
        case r9:
            switch (getMode())
            {
                case Mode::FIQ: return registers.r9_fiq;
                default: return registers.r9;
            }
        case r10:
            switch (getMode())
            {
                case Mode::FIQ: return registers.r10_fiq;
                default: return registers.r10;
            }
        case r11:
            switch (getMode())
            {
                case Mode::FIQ: return registers.r11_fiq;
                default: return registers.r11;
            }
        case r12:
            switch (getMode())
            {
                case Mode::FIQ: return registers.r12_fiq;
                default: return registers.r12;
            }

        case r13:
            switch(getMode())
            {
                case Mode::USR:
                case Mode::SYS: return registers.r13;
                case Mode::FIQ: return registers.r13_fiq;
                case Mode::SVC: return registers.r13_svc;
                case Mode::ABT: return registers.r13_abt;
                case Mode::IRQ: return registers.r13_irq;
                case Mode::UND: return registers.r13_und;
            }

        case r14:
            switch(getMode())
            {
                case Mode::USR:
                case Mode::SYS: return registers.r14;
                case Mode::FIQ: return registers.r14_fiq;
                case Mode::SVC: return registers.r14_svc;
                case Mode::ABT: return registers.r14_abt;
                case Mode::IRQ: return registers.r14_irq;
                case Mode::UND: return registers.r14_und;
            }

        case r15:
            return registers.r15; // all banks share r15
        case cpsr:
            return registers.cpsr.raw; // all banks share cpsr
        case spsr:
            switch(getMode())
            {
                case Mode::FIQ: return registers.spsr_fiq.raw;
                case Mode::SVC: return registers.spsr_svc.raw;
                case Mode::ABT: return registers.spsr_abt.raw;
                case Mode::IRQ: return registers.spsr_irq.raw;
                case Mode::SYS: return registers.cpsr.raw;
                case Mode::UND: return registers.spsr_und.raw;
            }
            break;
        default:
            assert(!"Error: unknown register in Arm7::getRegister");
            return -1;
    }

    assert(!"Error: unknown register in Arm7::getRegister");
    return -1;
}

void Arm7::setRegister(u32 reg, u32 val)
{
    switch (reg)
    {
        // all banks share r0 - r7
        case r0: registers.r0 = val; break;
        case r1: registers.r1 = val; break;
        case r2: registers.r2 = val; break;
        case r3: registers.r3 = val; break;
        case r4: registers.r4 = val; break;
        case r5: registers.r5 = val; break;
        case r6: registers.r6 = val; break;
        case r7: registers.r7 = val; break;

        // banked registers
        case r8:
            switch (getMode())
            {
                case Mode::FIQ:
                    registers.r8_fiq = val;
                    break;
                default:
                    registers.r8 = val;
                    break;
            }
            break;
        case r9:
            switch (getMode())
            {
                case Mode::FIQ:
                    registers.r9_fiq = val;
                    break;
                default:
                    registers.r9 = val;
                    break;
            }
            break;
        case r10:
            switch (getMode())
            {
                case Mode::FIQ:
                    registers.r10_fiq = val;
                    break;
                default:
                    registers.r10 = val;
                    break;
            }
            break;
        case r11:
            switch (getMode())
            {
                case Mode::FIQ:
                    registers.r11_fiq = val;
                    break;
                default:
                    registers.r11 = val;
                    break;
            }
            break;
        case r12:
            switch (getMode())
            {
                case Mode::FIQ:
                    registers.r12_fiq = val;
                    break;
                default:
                    registers.r12 = val;
                    break;
            }
            break;

        case r13:
            switch(getMode())
            {
                case Mode::USR:
                case Mode::SYS:
                    registers.r13 = val;
                    break;
                case Mode::FIQ:
                    registers.r13_fiq = val;
                    break;
                case Mode::SVC:
                    registers.r13_svc = val;
                    break;
                case Mode::ABT:
                    registers.r13_abt = val;
                    break;
                case Mode::IRQ:
                    registers.r13_irq = val;
                    break;
                case Mode::UND:
                    registers.r13_und = val;
                    break;
            }
            break;

        case r14:
            switch(getMode())
            {
                case Mode::USR:
                case Mode::SYS:
                    registers.r14 = val;
                    break;
                case Mode::FIQ:
                    registers.r14_fiq = val;
                    break;
                case Mode::SVC:
                    registers.r14_svc = val;
                    break;
                case Mode::ABT:
                    registers.r14_abt = val;
                    break;
                case Mode::IRQ:
                    registers.r14_irq = val;
                    break;
                case Mode::UND:
                    registers.r14_und = val;
                    break;
            }
            break;


        case r15:  registers.r15      = val; break; // all banks share r15
        case cpsr: registers.cpsr.raw = val; break; // all banks share cpsr
        default:
            assert(!"Error: unknown register in Arm7::setRegister");
    }
}

// update cpsr flags after a logical operation
void Arm7::updateFlagsLogical(u32 result, u8 carry_out)
{
    // C flag will be set to the carry out from the barrel shifter
    setConditionCodeFlag(ConditionFlag::C, carry_out);

    // Z flag will be set if and only if the result is all zeros
    uint8_t new_z = result == 0 ? 1 : 0;
    setConditionCodeFlag(ConditionFlag::Z, new_z);

    // N flag will be set to the logical value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    setConditionCodeFlag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary
}

// update cpsr flags after an addition operation
void Arm7::updateFlagsAddition(u32 op1, u32 op2, u32 result)
{
    // C flag will be set to the carry out of bit 31 of the ALU
    if (op1 > result || op2 > result)
        setConditionCodeFlag(ConditionFlag::C, 1);
    else
        setConditionCodeFlag(ConditionFlag::C, 0);

    // Z flag will be set if and only if the result was zero
    u8 new_z = result == 0 ? 1 : 0;
    setConditionCodeFlag(ConditionFlag::Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    u8 new_n = result & 0x80000000 ? 1 : 0;
    setConditionCodeFlag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    u8 op1_msb    = op1    & 0x80000000 ? 1 : 0;
    u8 op2_msb    = op2    & 0x80000000 ? 1 : 0;
    u8 result_msb = result & 0x80000000 ? 1 : 0;

    if (op1_msb == 0 && op2_msb == 0 && result_msb == 1)
        setConditionCodeFlag(ConditionFlag::V, 1);
    else if (op1_msb == 1 && op2_msb == 1 && result_msb == 0)
        setConditionCodeFlag(ConditionFlag::V, 1);
    else
        setConditionCodeFlag(ConditionFlag::V, 0);
}

// update cpsr flags after a subtraction operation
void Arm7::updateFlagsSubtraction(u32 op1, u32 op2, u32 result)
{
    // C flag will be set to the carry out of bit 31 of the ALU
    // ARM uses an inverted carry flag for borrow
    if (result > op1 || (result == op1 && op2 != 0))
        setConditionCodeFlag(ConditionFlag::C, 0);
    else
        setConditionCodeFlag(ConditionFlag::C, 1);

    // Z flag will be set if and only if the result was zero
    uint8_t new_z = result == 0 ? 1 : 0;
    setConditionCodeFlag(ConditionFlag::Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    setConditionCodeFlag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    u8 op1_msb    =    op1 & 0x80000000 ? 1 : 0;
    u8 op2_msb    =    op2 & 0x80000000 ? 1 : 0;
    u8 result_msb = result & 0x80000000 ? 1 : 0;

    if (op1_msb == 0 && op2_msb == 1 && result_msb == 1)
        setConditionCodeFlag(ConditionFlag::V, 1);
    else if (op1_msb == 1 && op2_msb == 0 && result_msb == 0)
        setConditionCodeFlag(ConditionFlag::V, 1);
    else
        setConditionCodeFlag(ConditionFlag::V, 0);

}

/* performs a shift operation on op2.
 * returns the carry-out of the barrel shifter
 *
 *  @params:
 *  shift_amount - the amount of times to shift
 *  num - the number that will actually be shifted
 *  opcode - an encoding of which type of shift to be performed
 */
u8 Arm7::barrelShift(u32 shift_amount, u32 &num, u8 opcode)
{
    u8 carry_out = getConditionCodeFlag(ConditionFlag::C); // preserve C flag

    // if shift_amount is 0, leave num unchanged and return the old shift flag
    if (shift_amount == 0)
        return carry_out;

    // # of bits in a word (should be 32)
    size_t num_bits = sizeof(u32) * 8;

    // perform shift
    switch (opcode)
    {
        // LSL
        case 0b00:
            if (shift_amount > num_bits) // undefined behavior to try to shift by more than 32
            { 
                num = 0;
                carry_out = 0;
            }
            
            else
            {
                num <<= (shift_amount - 1);
                carry_out = (num >> num_bits - 1) & 1; // most significant bit
                num <<= 1; // shift last time
            }
            break;
        
        // LSR
        case 0b01:
            if (shift_amount > num_bits) // undefined behavior to try to shift by more than 32s
            { 
                num = 0;
                carry_out = 0;
            }
            
            else
            {
                num >>= (shift_amount - 1);
                carry_out = num & 1;
                num >>= 1;
            }
            break;
        
        // ASR
        case 0b10:
            for (int i = 0; i < shift_amount; ++i)
            {
                carry_out  = num & 1;
                uint8_t msb = (num >> num_bits - 1) & 1; // most significant bit
                num >>= 1;
                num |= (msb << num_bits - 1);
            }
            break;
        
        // ROR
        case 0b11:
            if (shift_amount == 0xFFFFFFFF) // rotate right extended
            { 
                carry_out = num & 1;
                num >>= 1;
                num |= getConditionCodeFlag(ConditionFlag::C) << (num_bits - 1);
            }
            
            else // normal rotate right
            {
                for (int i = 0; i < shift_amount; ++i)
                {
                    carry_out = num & 1;
                    uint8_t dropped_lsb = num & 1;  
                    num >>= 1;
                    num |= (dropped_lsb << num_bits - 1);
                }
            }
            break;
    }

    return carry_out;
}

inline void Arm7::incrementPC()
{
    registers.r15 += getState() == State::ARM ? 4 : 2;
}

/*
 * Updates the value in the cpsr
 * Can also change the emulator's state or mode depending on the value
 */ 
void Arm7::updateCPSR(u32 value, bool flags_only)
{
    StatusRegister sr;
    sr.raw = value;

    // in user mode, only condition bits can be changed
    if (flags_only || getMode() == Mode::USR)
    {
        registers.cpsr.n = sr.n;
        registers.cpsr.z = sr.z;
        registers.cpsr.c = sr.c;
        registers.cpsr.v = sr.v;
        return;
    }

    registers.cpsr.raw = value;

    if (registers.cpsr.t != sr.t)
        log(LogLevel::Warning, "Software is changing T-Bit in CPSR!\n");

    // validate CPSR wasn't given an invalid state
    assert(getMode());

    // if (sr.state == IRQ && registers.cpsr.i == 1) return; // irq disabled bit set
    // if (sr.state == FIQ && registers.cpsr.f == 1) return; // fiq disabled bit set
}

/*
 * Updates the value in the spsr <mode>
 */ 
void Arm7::updateSPSR(u32 value, bool flags_only)
{
    StatusRegister old_spsr;

    // get spsr_<mode>
    switch (getMode())
    {
        // spsr doesn't exist in user mode
        case Mode::USR: std::cerr << "Error: SPSR does not exist in user mode" << "\n"; exit(6);
        case Mode::FIQ: old_spsr = registers.spsr_fiq; break;
        case Mode::SVC: old_spsr = registers.spsr_svc; break;
        case Mode::ABT: old_spsr = registers.spsr_abt; break;
        case Mode::IRQ: old_spsr = registers.spsr_irq; break;
        case Mode::SYS: old_spsr = registers.cpsr;     break;
        case Mode::UND: old_spsr = registers.spsr_und; break;
    }

    // new spsr
    StatusRegister new_spsr;
    new_spsr.raw = value;

    // don't have to check for USR mode b/c that was done above
    if (flags_only)
    {
        old_spsr.n = new_spsr.n;
        old_spsr.z = new_spsr.z;
        old_spsr.c = new_spsr.c;
        old_spsr.v = new_spsr.v;

        // set updated spsr_<mode>
        switch (getMode())
        {
            case Mode::FIQ: registers.spsr_fiq = old_spsr; break;
            case Mode::SVC: registers.spsr_svc = old_spsr; break;
            case Mode::ABT: registers.spsr_abt = old_spsr; break;
            case Mode::IRQ: registers.spsr_irq = old_spsr; break;
            case Mode::UND: registers.spsr_und = old_spsr; break;
            case Mode::SYS: log("SYS in SPSR flags\n");
        }
        
        return;
    }

    // set updated spsr_<mode>
    switch (getMode())
    {
        case Mode::FIQ: registers.spsr_fiq = new_spsr; break;
        case Mode::SVC: registers.spsr_svc = new_spsr; break;
        case Mode::ABT: registers.spsr_abt = new_spsr; break;
        case Mode::IRQ: registers.spsr_irq = new_spsr; break;
        case Mode::UND: registers.spsr_und = new_spsr; break;
        case Mode::SYS: log("SYS in SPSR\n");    break;
    }
}

// advances the cpu clock
// address is the current access address
// type is the cycle type, either 'n', 's', or 'i'
void Arm7::tick(u8 n, u8 s, u8 i)
{
    u16 access_cycles = 0;

    // non-sequential wait states
    for (int x = 0; x < n; ++x)
        access_cycles += 1 + mem->n_cycles; // 1 + N waitstates
    
    // sequential wait states
    for (int x = 0; x < s; ++x)
        access_cycles += 1 + mem->s_cycles; // 1 + S waitstates
    
    // internal cycle
    for (int x = 0; x < i; ++x)
        access_cycles += 1; // 1 cycle
    
    // keep running total of cycles
    cycles += access_cycles;
}

void Arm7::handleInterrupt()
{
    // exit interrupt
    if (in_interrupt && getRegister(r15) == 0x138)
    {
        //std::cout << "Handled interrupt!\n";

        // restore registers from stack
        // ldmfd r13! r0-r3, r12, r14
        u32 sp = getRegister(r13);
        setRegister(r0,  mem->read32(sp)); sp += 4;
        setRegister(r1,  mem->read32(sp)); sp += 4;
        setRegister(r2,  mem->read32(sp)); sp += 4;
        setRegister(r3,  mem->read32(sp)); sp += 4;
        setRegister(r12, mem->read32(sp)); sp += 4;
        setRegister(r14, mem->read32(sp)); sp += 4;
        setRegister(r13, sp);

        // return from IRQ
        // subs r15, r14, 4
        setRegister(r15, getRegister(r14) - 4);

        // restore CPSR
        setRegister(cpsr, getRegister(spsr));

        // re-enable interrupts
        registers.cpsr.i = 0;
        irq->enable();

        pipeline_full = false;
        in_interrupt  = false;

        last_read_bios = bios_read_state[2];
        
        return;
    }

    // check if master interrupts are enabled
    if (irq->isEnabled() && registers.cpsr.i == 0) 
    {
        //std::cout << "enabled\n";
        // get enabled interrupts and requested interrupts
        // u16 interrupts_enabled   = mem->Read16Unsafe(REG_IE);
        // u16 interrupts_requested = mem->Read16Unsafe(REG_IF);
        //auto irq_mask = mem->read16Unsafe(REG_IE) & mem->read16Unsafe(REG_IF);

        u16 irq_mask = irq->getIE() & irq->getIF();

        // get first identical set bit in enabled/requested interrupts
        for (int i = 0; i < 14; ++i) // 14 interrupts available
        {
            // handle interrupt at position i
            if (irq_mask & (1 << i))
            {
                //std::cout << "IRQ\n";
                //LLE interrupts through BIOS
                // registers.spsr_irq = registers.cpsr;

                // if (GetState() == State::ARM)
                //     SetRegister(r14, GetRegister(r15) - 4);
                // else 
                //     SetRegister(r14, GetRegister(r15) - 2);

                // SetMode(Mode::IRQ);
                // SetState(State::ARM);
                // registers.cpsr.i = 1;
                // mem->Write32Unsafe(REG_IME, 0);
                // SetRegister(r15, 0x1C);
                // pipeline_full = false;
                // in_interrupt = true;

                // emulate how BIOS handles interrupts - HLE
                //std::cout << "interrupt handling! " << i << "\n";

                u32 old_cpsr = getRegister(cpsr);
                // switch to IRQ
                setMode(Mode::IRQ);

                // save CPSR to SPSR
                updateSPSR(old_cpsr, false);
                
                // no branch
                if (pipeline_full)
                {
                    if (getState() == State::ARM) {
                        //std::cout << "arm interrupt\n";
                        setRegister(r14, getRegister(r15) - 4);
                    }
                    else {
                        //std::cout << "thumb interrupt\n";
                        setRegister(r14, getRegister(r15));
                    }
                }

                // branch
                else
                {
                    //std::cout << "Caution: interrupt after a branch\n";
                    setRegister(r14, getRegister(r15) + 4);
                }

                // save registers to SP_irq
                // stmfd  r13!, r0-r3, r12, r14
                u32 sp = getRegister(r13);
                sp -= 4; mem->write32(sp, getRegister(r14)); 
                sp -= 4; mem->write32(sp, getRegister(r12));
                sp -= 4; mem->write32(sp, getRegister(r3));
                sp -= 4; mem->write32(sp, getRegister(r2));
                sp -= 4; mem->write32(sp, getRegister(r1));
                sp -= 4; mem->write32(sp, getRegister(r0));
                setRegister(r13, sp);

                // mov r0, 0x4000000
                setRegister(r0, 0x4000000);

                // address where BIOS returns from IRQ handler
                setRegister(r14, 0x138);

                // ldr r15, [r0, -0x4]
                setRegister(r15, mem->read32(getRegister(r0) - 0x4) & ~0x3);

                // disable interrupts
                registers.cpsr.i = 1;
                irq->disable();

                setState(State::ARM);
                pipeline_full = false;
                in_interrupt  = true;

                last_read_bios = bios_read_state[1];

                // save the current interrupt so we can clear it after it's been serviced
                return;
            }
        }
    }
}
int i = 0xFFFF;

u32 Arm7::read8(u32 address)
{
    // reading from BIOS memory
    if (address <= 0x3FFF && registers.r15 > 0x3FFF)
    {
        log(LogLevel::Error, "Invalid read from BIOS u8: {0:#x}\n", last_read_bios);
        return last_read_bios & 0xFF;
    }

    if ((address >= 0x4000 && address <= 0x1FFFFFF) || address >= 0x10000000)
    {
        log(LogLevel::Warning, "UNUSED U8\n");
        return readUnused(address);
    }

    return mem->read8(address);
}

/*
 * Reads a halfword from the specified memory address
 * pass true if the halfword is signed, false otherwise
 * This needs to be known for misalignment reasons
 */
u32 Arm7::read16(u32 address, bool sign)
{
    // reading from BIOS memory
    if (address <= 0x3FFF && registers.r15 > 0x3FFF)
    {
        //log(LogLevel::Error, "Invalid read from BIOS u16: 0x{x}\n", last_read_bios);

        u32 value = last_read_bios;
        return value & 0xFFFF;
    }

    bool valid = true;

    switch (address)
    {
        case REG_BG0HOFS:
        case REG_BG1HOFS:
        case REG_BG2HOFS:
        case REG_BG3HOFS:
        case REG_BG0VOFS:
        case REG_BG1VOFS:
        case REG_BG2VOFS:
        case REG_BG3VOFS:
        case REG_BG2X:
        case REG_BG2Y:
        case REG_BG2X + 2:
        case REG_BG2Y + 2:
        case REG_BG2PA:
        case REG_BG2PB:
        case REG_BG2PC:
        case REG_BG2PD:
        case REG_BG3X:
        case REG_BG3Y:
        case REG_BG3X + 2:
        case REG_BG3Y + 2:
        case REG_BG3PA:
        case REG_BG3PB:
        case REG_BG3PC:
        case REG_BG3PD:
        case REG_WIN0H:
        case REG_WIN1H:
        case REG_WIN0V:
        case REG_WIN1V:
        case REG_MOSAIC:
        case REG_MOSAIC + 2:
        case REG_DMA0SAD:
        case REG_DMA0DAD:
        case REG_DMA0CNT:
        case REG_DMA1SAD:
        case REG_DMA1DAD:
        case REG_DMA1CNT:
        case REG_DMA2SAD:
        case REG_DMA2DAD:
        case REG_DMA2CNT:
        case REG_DMA3SAD:
        case REG_DMA3DAD:
        case REG_DMA3CNT:
            return readUnused(address);
    }

    if ((address >= 0x4000 && address <= 0x1FFFFFF) || address >= 0x10000000)
    {
        std::cout << "UNUSED U16\n";
        return readUnused(address);
    }

    u32 data;

    if (sign)
    {
        data = (u32) mem->read16(address);

        // misaligned address, sign extend BYTE value
        if ((address & 1) != 0)
        {
            if (data & 0x80)
                data |= 0xFFFFFF00;
        }
        
        // correctly aligned address, sign extend HALFWORD value
        else
        {
            if (data & 0x8000)
                data |= 0xFFFF0000;
        }
    }
    
    else
    {
        // read from forcibly aligned address
        data = (u32) mem->read16(address & ~1);
        // misaligned read - reads from forcibly aligned address "addr AND 1", and does then rotate the data as "ROR 8"
        if ((address & 1) != 0)
            barrelShift(8, data, 0b11);
    }
    
    return data;    
}

/*
 * Reads a word from the specified memory address
 * pass true if this is a LDR or SWP operation false otherwise
 * This needs to be known for misalignment reasons
 */
u32 Arm7::read32(u32 address, bool ldr)
{
    // reading from BIOS memory
    if (address <= 0x3FFF)
    {

        if (registers.r15 < 0x3FFF)
            last_read_bios = mem->read32Unsafe(address);
        
        return last_read_bios;
    }

    switch (address)
    {
        [[unlikely]] case REG_DMA0CNT:
        [[unlikely]] case REG_DMA1CNT:
        [[unlikely]] case REG_DMA2CNT:
        [[unlikely]] case REG_DMA3CNT:
            return mem->read32Unsafe(address) & 0x00FFFFFF; // return 0 for unused byte 0x40000XE
    }

    if ((address >= 0x4000 && address <= 0x1FFFFFF) || address >= 0x10000000)
    {
        std::cout << "UNUSED U32\n";
        return readUnused(address);
    }
    
    // read from forcibly aligned address
    u32 data = mem->read32(address & ~3);

    // misaligned read - reads from forcibly aligned address "addr AND (NOT 3)", and does then rotate the data as "ROR (addr AND 3)*8"
    // only used for LDR and SWP operations, otherwise just use data from forcibly aligned address
    if (ldr && ((address & 3) != 0))
        barrelShift((address & 3) << 3, data, 0b11);

    // 8 cycles for gamepak rom access, 5 from mem_check and 3 here
    // if (address >= MEM_GAMEPAK_ROM_START && address <= MEM_GAMEPAK_ROM_END)
    //     cycles += 3;

    return data;
}


void Arm7::write8(u32 address, u8 value)
{
    if (!memCheckWrite(address)) return;

    // byte write to Palette RAM is written in both upper and lower 8 bytes of halfword at address
    if (address >= MEM_PALETTE_RAM_START && address <= MEM_PALETTE_RAM_END)
    {
        mem->write8(address, value);
        mem->write8(address + 1, value);
        return;
    }
    
    // byte write to OAM is ignored
    if (address >= MEM_OAM_START && address <= MEM_OAM_END)
        return;

    // VRAM byte writes
    if (address >= MEM_VRAM_START && address <= MEM_VRAM_END)
    {      
        switch (mem->stat->dispcnt.mode)
        {
            // tile modes
            case 0:
            case 1:
            case 2:
                // writes to OBJ (0x6010000-0x6017FFF) are ignored
                if (address >= 0x6010000)
                    return;
                
                // writes to BG (0x6000000-0x6013FFF) write to both upper and lower bits
                mem->write8(address, value);
                mem->write8(address + 1, value);

                break;

            // bitmap modes
            case 3:
            case 4:
            case 5:
                // writes to OBJ (0x6014000-0x6017FFF) are ignored
                if (address >= 0x6014000)
                    return;
                
                // writes to BG (0x6000000-0x6013FFF) write to both upper and lower bits
                mem->write8(address, value);
                mem->write8(address + 1, value);

                break;
        }

        return;
    }

    // normal byte write
    mem->write8(address, value);
}

void Arm7::write16(u32 address, u16 value)
{
    // align address to halfword
    address &= ~0x1;

    if (!memCheckWrite(address)) return;
    mem->write16(address, value);
}

void Arm7::write32(u32 address, u32 value)
{
    // align address to word
    address &= ~0x3;

    if (!memCheckWrite(address)) return;

    // 8 cycles for gamepak rom access, 5 from mem_check and 3 here
    // if (address >= MEM_GAMEPAK_ROM_START && address <= MEM_GAMEPAK_ROM_END)
    //     cycles += 3;
    mem->write32(address, value);
}

// determine if a read at the specified address is allowed
inline bool Arm7::memCheckRead(u32 &address)
{
    // upper 4 bits of address bus are unused
    // if (address >= 0x10000000)
    //     return false;

    // // add cycles for expensive memory accesses

    // // +1 cycles for VRAM accress while not in v-blank
    // if (address >= MEM_PALETTE_RAM_START && address <= MEM_OAM_END && !mem->stat->dispstat.in_vBlank)
    //     cycles++;
    
    // // bios read
    // if (address <= 0x3FFF)
    // {
    //     if (registers.r15 >= 0x3FFF)
    //         return false;
        
    //     last_read_bios = mem->read_u32_unprotected(address);
    // }

    // // Reading from Unused or Write-Only I/O Ports
    // // Works like above Unused Memory when the entire 32bit memory fragment is Unused (eg. 0E0h)
    // // and/or Write-Only (eg. DMA0SAD). And otherwise, returns zero if the lower 16bit fragment is readable (eg. 04Ch=MOSAIC, 04Eh=NOTUSED/ZERO).
    // switch (address)
    // {
    //     case REG_DMA0CNT:
    //     case REG_DMA1CNT:
    //     case REG_DMA2CNT:
    //     case REG_DMA3CNT:
    //         return false;
    //     default:
    //         break;
    // }
    
    return true;
}

u32 Arm7::readUnused(u32 address)
{
    if (getState() == State::ARM)
    {
        return mem->read32(dollar() + 8);
    }

    else
    {
        log(LogLevel::Warning, "Reading unused in thumb mode!\n");
        u16 lsw, msw;
        switch (Memory::getMemoryRegion(address))
        {
            case Memory::Region::EWRAM:
            case Memory::Region::PALRAM:
            case Memory::Region::VRAM:
            case Memory::Region::ROM:
                lsw = mem->read16(dollar() + 4);
                msw = mem->read16(dollar() + 4);
                break;
            
            case Memory::Region::BIOS:
            case Memory::Region::OAM:
                // address is 4 byte aligned
                if ((address & 0x3) == 0)
                {
                    lsw = mem->read16(dollar() + 4);
                    msw = mem->read16(dollar() + 6);
                }

                else
                {
                    lsw = mem->read16(dollar() + 2);
                    msw = mem->read16(dollar() + 4);
                }

                break;
            
            case Memory::Region::IWRAM:
                // address is 4 byte aligned
                if ((address & 0x3) == 0)
                {
                    lsw = mem->read16(dollar() + 4);
                    msw = mem->read16(dollar() + 2);
                }

                else
                {
                    lsw = mem->read16(dollar() + 2);
                    msw = mem->read16(dollar() + 4);
                }

                break;
            
            case Memory::Region::MMIO:
                lsw = mem->read16(dollar() + 4);
                msw = mem->read16(dollar() + 4);
                break;
            
            default:
                log(LogLevel::Error, "Invalid unused thumb read\n");
                return 0;
        }

        return msw << 16 | lsw;
    }
}

// determine if a write at the specified address is allowed
inline bool Arm7::memCheckWrite(u32 &address)
{
    // upper 4 bits of address bus are unused, so mirror it if trying to access
    if (address >= 0x10000000)
        address &= 0x0FFFFFFF;

    // add cycles for expensive memory accesses

    // +1 cycles for VRAM accress while not in v-blank
    if (address >= MEM_PALETTE_RAM_START && address <= MEM_OAM_END && !mem->stat->dispstat.in_vBlank)
        cycles++;
    
    // bios write
    if (address <= 0x3FFF)
        return false;
    
    return true;
}

bool Arm7::checkState()
{
    bool valid = false;

    switch (getMode())
    {
        case Mode::USR:
        case Mode::FIQ:
        case Mode::IRQ:
        case Mode::SVC:
        case Mode::ABT:
        case Mode::SYS:
        case Mode::UND:
            valid = true;
    }

    return valid;
}

void Arm7::print() {
    std::cout<< std::hex <<"R0 : 0x" << std::setw(8) << std::setfill('0') << getRegister(0) << 
				" -- R4  : 0x" << std::setw(8) << std::setfill('0') << getRegister(4) << 
				" -- R8  : 0x" << std::setw(8) << std::setfill('0') << getRegister(8) << 
				" -- R12 : 0x" << std::setw(8) << std::setfill('0') << getRegister(12) << "\n";

			std::cout<< std::hex <<"R1 : 0x" << std::setw(8) << std::setfill('0') << getRegister(1) << 
				" -- R5  : 0x" << std::setw(8) << std::setfill('0') << getRegister(5) << 
				" -- R9  : 0x" << std::setw(8) << std::setfill('0') << getRegister(9) << 
				" -- R13 : 0x" << std::setw(8) << std::setfill('0') << getRegister(13) << "\n";

			std::cout<< std::hex <<"R2 : 0x" << std::setw(8) << std::setfill('0') << getRegister(2) << 
				" -- R6  : 0x" << std::setw(8) << std::setfill('0') << getRegister(6) << 
				" -- R10 : 0x" << std::setw(8) << std::setfill('0') << getRegister(10) << 
				" -- R14 : 0x" << std::setw(8) << std::setfill('0') << getRegister(14) << "\n";

			std::cout<< std::hex <<"R3 : 0x" << std::setw(8) << std::setfill('0') << getRegister(3) << 
				" -- R7  : 0x" << std::setw(8) << std::setfill('0') << getRegister(7) << 
				" -- R11 : 0x" << std::setw(8) << std::setfill('0') << getRegister(11) << 
				" -- R15 : 0x" << std::setw(8) << std::setfill('0') << getRegister(15) << "\n";

	
			std::cout<< std::hex <<"CPSR : 0x" << std::setw(8) << std::setfill('0') << registers.cpsr.raw << "\t";
            if (getConditionCodeFlag(ConditionFlag::N))
                std::cout << "N";
            if (getConditionCodeFlag(ConditionFlag::Z))
                std::cout << "Z";
            if (getConditionCodeFlag(ConditionFlag::C))
                std::cout << "C";
            if (getConditionCodeFlag(ConditionFlag::V))
                std::cout << "V";
            std::cout << "\n";
            //std:: cout << std::dec << ii << " instructions\n";
    }

// #include "HandlerArm.cpp"
// #include "HandlerThumb.cpp"
// #include "swi.cpp"