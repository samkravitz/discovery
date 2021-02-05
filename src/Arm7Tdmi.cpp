/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Arm7Tdmi.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of arm7tdmi functions
 */
#include <iostream>
#include <iomanip>

#include "Arm7Tdmi.h"

// uncomment this if running tests
//#define TEST

//#define PRINT

Arm7Tdmi::Arm7Tdmi(Memory *mem) : mem(mem)
{
    registers = {0}; // zero out registers
    registers.r15 = 0x8000000; // starting address of gamepak flash rom

    registers.r13     = 0x3007F00; // starting address of user stack
    registers.r13_svc = 0x3007FE0; // starting address of swi stack
    registers.r13_irq = 0x3007FA0; // starting address of interrupt stack

    SetMode(Mode::SVC);
    SetState(State::ARM);

    // initialize cpsr
    registers.cpsr.flags.f = 1;
    registers.cpsr.flags.i = 1;

    pipeline_full = false;
    cycles = 0;
    current_interrupt = 0;
    in_interrupt  = false;
    swi_vblank_intr = false;
    last_read_bios = 0xE129F000;
    
    // different initialization for the testing environment
    #ifdef TEST
    registers.r15 = 0;
    set_state(USR);
    mem = new Memory();
    #endif
}

Arm7Tdmi::~Arm7Tdmi() { }

Mode Arm7Tdmi::GetMode()
{
    switch (registers.cpsr.flags.mode)
    {
        case 0b10000: return Mode::USR;
        case 0b10001: return Mode::FIQ;
        case 0b10010: return Mode::IRQ;
        case 0b10011: return Mode::SVC;
        case 0b10111: return Mode::ABT;
        case 0b11111: return Mode::SYS;
        case 0b11011: return Mode::UND;
        default:
            LOG(LogLevel::Error, "Undefined mode {}\n", (int) registers.cpsr.flags.mode);
            exit(21);
    } 
}

void Arm7Tdmi::SetMode(Mode mode)
{
    bool valid = false;

    switch (mode)
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

    switch (mode)
    {
        case Mode::USR: registers.cpsr.flags.mode = 0b10000; break;
        case Mode::FIQ: registers.cpsr.flags.mode = 0b10001; break;
        case Mode::IRQ: registers.cpsr.flags.mode = 0b10010; break;
        case Mode::SVC: registers.cpsr.flags.mode = 0b10011; break;
        case Mode::ABT: registers.cpsr.flags.mode = 0b10111; break;
        case Mode::SYS: registers.cpsr.flags.mode = 0b11111; break;
        case Mode::UND: registers.cpsr.flags.mode = 0b11011; break;
    }
}

u8 Arm7Tdmi::GetConditionCodeFlag(ConditionFlag flag)
{
    switch (flag)
    {
        case ConditionFlag::N: return registers.cpsr.flags.n; 
        case ConditionFlag::Z: return registers.cpsr.flags.z;
        case ConditionFlag::C: return registers.cpsr.flags.c;
        case ConditionFlag::V: return registers.cpsr.flags.v;
        default:
            std::cerr << "Unrecognized condition code flag\n";
            return 0;
    }
}

void Arm7Tdmi::SetConditionCodeFlag(ConditionFlag flag, u8 bit)
{
    // bit can only be 0 or 1
    if (bit > 1)
    {
        std::cerr << "Error: set bit must be 0 or 1, it is: " << bit << "\n";
        return;
    }

    switch (flag)
    {
        case ConditionFlag::N: registers.cpsr.flags.n = bit; break;
        case ConditionFlag::Z: registers.cpsr.flags.z = bit; break;
        case ConditionFlag::C: registers.cpsr.flags.c = bit; break;
        case ConditionFlag::V: registers.cpsr.flags.v = bit; break;
        default:
            std::cerr << "Unrecognized condition code flag\n";
            return;
    }
}

// determine if the condition field of an instruction is true, given the state of the CPSR
bool Arm7Tdmi::ConditionMet(Condition condition)
{
    switch (condition)
    {
        case Condition::EQ: return GetConditionCodeFlag(ConditionFlag::Z);  // Z set
        case Condition::NE: return !GetConditionCodeFlag(ConditionFlag::Z); // Z clear
        case Condition::CS: return GetConditionCodeFlag(ConditionFlag::C);  // C set
        case Condition::CC: return !GetConditionCodeFlag(ConditionFlag::C); // C clear
        case Condition::MI: return GetConditionCodeFlag(ConditionFlag::N);  // N set
        case Condition::PL: return !GetConditionCodeFlag(ConditionFlag::N); // N Clear
        case Condition::VS: return GetConditionCodeFlag(ConditionFlag::V);  // V set
        case Condition::VC: return !GetConditionCodeFlag(ConditionFlag::V); // V clear
        case Condition::HI: return GetConditionCodeFlag(ConditionFlag::C) && !GetConditionCodeFlag(ConditionFlag::Z); // C set and Z clear
        case Condition::LS: return !GetConditionCodeFlag(ConditionFlag::C) || GetConditionCodeFlag(ConditionFlag::Z); // C clear or Z set
        case Condition::GE: return GetConditionCodeFlag(ConditionFlag::N) == GetConditionCodeFlag(ConditionFlag::V);  // N equals V
        case Condition::LT: return GetConditionCodeFlag(ConditionFlag::N) != GetConditionCodeFlag(ConditionFlag::V);  // N not equal V
        case Condition::GT: return !GetConditionCodeFlag(ConditionFlag::Z) && (GetConditionCodeFlag(ConditionFlag::N) == GetConditionCodeFlag(ConditionFlag::V)); // Z clear AND (N equals V)
        case Condition::LE: return GetConditionCodeFlag(ConditionFlag::Z) || (GetConditionCodeFlag(ConditionFlag::N) != GetConditionCodeFlag(ConditionFlag::V));  // Z set OR (N not equal to V)
        case Condition::AL: return true; // always
        default: // should never happen
            std::cerr << "Unrecognized condition field\n";
            return false;
    }
}

void Arm7Tdmi::Fetch()
{
    if (!pipeline_full)
    {
        // fill pipeline
        switch (GetState())
        {
            case State::ARM:
                pipeline[0] = Read32(registers.r15, false); registers.r15 += 4;
                pipeline[1] = Read32(registers.r15, false); registers.r15 += 4;
                pipeline[2] = Read32(registers.r15, false);
                break;
            case State::THUMB:
                pipeline[0] = Read16(registers.r15, false); registers.r15 += 2;
                pipeline[1] = Read16(registers.r15, false); registers.r15 += 2;
                pipeline[2] = Read16(registers.r15, false);
                break;
        }

        pipeline_full = true;
        return;
    }

    switch (GetState())
    {
        case State::ARM:
            pipeline[2] = Read32(registers.r15, false);
            break;
        case State::THUMB:
            pipeline[2] = (u16) Read16(registers.r15, false);
            break;
    }
}

void Arm7Tdmi::Decode(u32 instruction) { }

void Arm7Tdmi::Execute(u32 instruction)
{  
    #ifdef PRINT
    std::cout << "Executing: " << std::hex << instruction << "\n";
    if (instruction == 0)
        exit(5);
    #endif
    
    switch (GetState())
    {
        case State::ARM:
            if (!ConditionMet((Condition) Util::bitseq<31, 28>(instruction)))
            {
                IncrementPC();
                Tick(0, 0, 1); // 1I
                return;
            }
            
            switch(Util::GetInstructionFormat(instruction))
            {
                case ArmInstruction::BEX:  BranchExchange(instruction);       break;
                case ArmInstruction::B:    BranchLink(instruction);           break;
                case ArmInstruction::DP:   DataProcessing(instruction);       break;
                case ArmInstruction::MUL:  Multiply(instruction);             break;
                case ArmInstruction::MULL: MultiplyLong(instruction);         break;
                case ArmInstruction::PSR:  PSRTransfer(instruction);          break;
                case ArmInstruction::SDT:  SingleDataTransfer(instruction);   break;
                case ArmInstruction::HDT:  HalfwordDataTransfer(instruction); break;
                case ArmInstruction::BDT:  BlockDataTransfer(instruction);    break;
                case ArmInstruction::SWP:  SingleDataSwap(instruction);       break;
                case ArmInstruction::INT:  SoftwareInterruptArm(instruction); break;
                default:
                    LOG(LogLevel::Error, "Cannot execute instruction {}, pc {}\n", instruction, registers.r15);
                    registers.r15 &= ~0x3;
            }
            break;

        case State::THUMB:
            u16 instr = (u16) instruction;
            switch(Util::GetInstructionFormat((u16) instruction))
            {
                case ThumbInstruction::MSR:    MoveShiftedRegister(instr);     break;
                case ThumbInstruction::ADDSUB: AddSubtract(instr);             break;
                case ThumbInstruction::IMM:    MoveImmediate(instr);           break;
                case ThumbInstruction::ALU:    AluThumb(instr);                break;
                case ThumbInstruction::HI:     HiRegisterOps(instr);           break;
                case ThumbInstruction::PC:     PcRelLoad(instr);               break;
                case ThumbInstruction::MOV:    LoadStoreRegOffset(instr);      break;
                case ThumbInstruction::MOVS:   LoadStoreSignedHalfword(instr); break;
                case ThumbInstruction::MOVI:   LoadStoreImmediate(instr);      break;
                case ThumbInstruction::MOVH:   LoadStoreHalfword(instr);       break;
                case ThumbInstruction::SP:     SpRelLoadStore(instr);          break;
                case ThumbInstruction::LDA:    LoadAddress(instr);             break;
                case ThumbInstruction::ADDSP:  AddOffsetToSp(instr);           break;
                case ThumbInstruction::POP:    PushPop(instr);                 break;
                case ThumbInstruction::MOVM:   MultipleLoadStore(instr);       break;
                case ThumbInstruction::B:      ConditionalBranch(instr);       break;
                case ThumbInstruction::SWI:    SoftwareInterruptThumb(instr);  break;
                case ThumbInstruction::BAL:    UnconditionalBranch(instr);     break;
                case ThumbInstruction::BL:     LongBranchLink(instr);          break;
                default:
                    std::cerr << "Cannot execute thumb instruction: " << (u16) instruction << " " << std::hex << registers.r15 << "\n";
                    registers.r15 &= ~0x1;
            }
            break;
    }

    // increment pc if there was no branch
    if (pipeline_full)
        IncrementPC();

    #ifdef PRINT
    std::cout<< std::hex <<"R0 : 0x" << std::setw(8) << std::setfill('0') << get_register(0) << 
				" -- R4  : 0x" << std::setw(8) << std::setfill('0') << get_register(4) << 
				" -- R8  : 0x" << std::setw(8) << std::setfill('0') << get_register(8) << 
				" -- R12 : 0x" << std::setw(8) << std::setfill('0') << get_register(12) << "\n";

			std::cout<< std::hex <<"R1 : 0x" << std::setw(8) << std::setfill('0') << get_register(1) << 
				" -- R5  : 0x" << std::setw(8) << std::setfill('0') << get_register(5) << 
				" -- R9  : 0x" << std::setw(8) << std::setfill('0') << get_register(9) << 
				" -- R13 : 0x" << std::setw(8) << std::setfill('0') << get_register(13) << "\n";

			std::cout<< std::hex <<"R2 : 0x" << std::setw(8) << std::setfill('0') << get_register(2) << 
				" -- R6  : 0x" << std::setw(8) << std::setfill('0') << get_register(6) << 
				" -- R10 : 0x" << std::setw(8) << std::setfill('0') << get_register(10) << 
				" -- R14 : 0x" << std::setw(8) << std::setfill('0') << get_register(14) << "\n";

			std::cout<< std::hex <<"R3 : 0x" << std::setw(8) << std::setfill('0') << get_register(3) << 
				" -- R7  : 0x" << std::setw(8) << std::setfill('0') << get_register(7) << 
				" -- R11 : 0x" << std::setw(8) << std::setfill('0') << get_register(11) << 
				" -- R15 : 0x" << std::setw(8) << std::setfill('0') << get_register(15) << "\n";

	
			std::cout<< std::hex <<"CPSR : 0x" << std::setw(8) << std::setfill('0') << registers.cpsr.raw << "\t";
            if (get_condition_code_flag(N))
                std::cout << "N";
            if (get_condition_code_flag(Z))
                std::cout << "Z";
            if (get_condition_code_flag(C))
                std::cout << "C";
            if (get_condition_code_flag(V))
                std::cout << "V";
            std::cout << "\n";
            //std:: cout << std::dec << ii << " instructions\n";
    #endif
}

u32 Arm7Tdmi::GetRegister(u32 reg)
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
            switch (GetMode())
            {
                case Mode::FIQ: return registers.r8_fiq;
                default: return registers.r8;
            }
        case r9:
            switch (GetMode())
            {
                case Mode::FIQ: return registers.r9_fiq;
                default: return registers.r9;
            }
        case r10:
            switch (GetMode())
            {
                case Mode::FIQ: return registers.r10_fiq;
                default: return registers.r10;
            }
        case r11:
            switch (GetMode())
            {
                case Mode::FIQ: return registers.r11_fiq;
                default: return registers.r11;
            }
        case r12:
            switch (GetMode())
            {
                case Mode::FIQ: return registers.r12_fiq;
                default: return registers.r12;
            }

        case r13:
            switch(GetMode())
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
            switch(GetMode())
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
            switch(GetMode())
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
            std::cerr << "Unknown register: " << reg << "\n";
            return 0;
    }
    return 100; // should never happen
}

void Arm7Tdmi::SetRegister(u32 reg, u32 val)
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
            switch (GetMode())
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
            switch (GetMode())
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
            switch (GetMode())
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
            switch (GetMode())
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
            switch (GetMode())
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
            switch(GetMode())
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
            switch(GetMode())
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


        case r15: registers.r15 = val; break; // all banks share r15
        case cpsr: registers.cpsr.raw = val; break; // all banks share cpsr
        default:
            std::cerr << "Unknown register: " << reg << "\n";
            break;
    }
}

// update cpsr flags after a logical operation
void Arm7Tdmi::UpdateFlagsLogical(u32 result, u8 carry_out)
{
    // C flag will be set to the carry out from the barrel shifter
    SetConditionCodeFlag(ConditionFlag::C, carry_out);

    // Z flag will be set if and only if the result is all zeros
    uint8_t new_z = result == 0 ? 1 : 0;
    SetConditionCodeFlag(ConditionFlag::Z, new_z);

    // N flag will be set to the logical value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    SetConditionCodeFlag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary
}

// update cpsr flags after an addition operation
void Arm7Tdmi::UpdateFlagsAddition(u32 op1, u32 op2, u32 result)
{
    // C flag will be set to the carry out of bit 31 of the ALU
    if (op1 > result || op2 > result)
        SetConditionCodeFlag(ConditionFlag::C, 1);
    else
        SetConditionCodeFlag(ConditionFlag::C, 0);

    // Z flag will be set if and only if the result was zero
    u8 new_z = result == 0 ? 1 : 0;
    SetConditionCodeFlag(ConditionFlag::Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    u8 new_n = result & 0x80000000 ? 1 : 0;
    SetConditionCodeFlag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    u8 op1_msb = op1 & 0x80000000 ? 1 : 0;
    u8 op2_msb = op2 & 0x80000000 ? 1 : 0;
    u8 result_msb = result & 0x80000000 ? 1 : 0;

    if (op1_msb == 0 && op2_msb == 0 && result_msb == 1)
        SetConditionCodeFlag(ConditionFlag::V, 1);
    else if (op1_msb == 1 && op2_msb == 1 && result_msb == 0)
        SetConditionCodeFlag(ConditionFlag::V, 1);
    else
        SetConditionCodeFlag(ConditionFlag::V, 0);
}

// update cpsr flags after a subtraction operation
void Arm7Tdmi::UpdateFlagsSubtraction(u32 op1, u32 op2, u32 result)
{
    // C flag will be set to the carry out of bit 31 of the ALU
    // ARM uses an inverted carry flag for borrow
    if (result > op1 || (result == op1 && op2 != 0))
        SetConditionCodeFlag(ConditionFlag::C, 0);
    else
        SetConditionCodeFlag(ConditionFlag::C, 1);

    // Z flag will be set if and only if the result was zero
    uint8_t new_z = result == 0 ? 1 : 0;
    SetConditionCodeFlag(ConditionFlag::Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    SetConditionCodeFlag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    u8 op1_msb = op1 & 0x80000000 ? 1 : 0;
    u8 op2_msb = op2 & 0x80000000 ? 1 : 0;
    u8 result_msb = result & 0x80000000 ? 1 : 0;

    if (op1_msb == 0 && op2_msb == 1 && result_msb == 1)
        SetConditionCodeFlag(ConditionFlag::V, 1);
    else if (op1_msb == 1 && op2_msb == 0 && result_msb == 0)
        SetConditionCodeFlag(ConditionFlag::V, 1);
    else
        SetConditionCodeFlag(ConditionFlag::V, 0);

}

/* performs a shift operation on op2.
 * returns the carry-out of the barrel shifter
 *
 *  @params:
 *  shift_amount - the amount of times to shift
 *  num - the number that will actually be shifted
 *  opcode - an encoding of which type of shift to be performed
 */
u8 Arm7Tdmi::BarrelShift(u32 shift_amount, u32 &num, u8 opcode)
{
    u8 carry_out = GetConditionCodeFlag(ConditionFlag::C); // preserve C flag

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
                num |= GetConditionCodeFlag(ConditionFlag::C) << (num_bits - 1);
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

inline void Arm7Tdmi::IncrementPC()
{
    registers.r15 += GetState() == State::ARM ? 4 : 2;
}

/*
 * Updates the value in the cpsr
 * Can also change the emulator's state or mode depending on the value
 */ 
void Arm7Tdmi::UpdateCPSR(u32 value, bool flags_only)
{
    StatusRegister sr;
    sr.raw = value;

    // in user mode, only condition bits can be changed
    if (flags_only || GetMode() == Mode::USR)
    {
        registers.cpsr.flags.n = sr.flags.n;
        registers.cpsr.flags.z = sr.flags.z;
        registers.cpsr.flags.c = sr.flags.c;
        registers.cpsr.flags.v = sr.flags.v;
        return;
    }

    registers.cpsr.raw = value;

    if (registers.cpsr.flags.t != sr.flags.t)
        LOG(LogLevel::Warning, "Software is changing T-Bit in CPSR!\n");

    // TODO - validate CPSR was appropriately changed
    bool valid = false;

    switch (GetMode())
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

    if (!valid)
        std::cerr << "Invalid state being set to cpsr: " << value << "\n";

    // if (sr.flags.state == IRQ && registers.cpsr.flags.i == 1) return; // irq disabled bit set
    // if (sr.flags.state == FIQ && registers.cpsr.flags.f == 1) return; // fiq disabled bit set
}

/*
 * Updates the value in the spsr <mode>
 */ 
void Arm7Tdmi::UpdateSPSR(u32 value, bool flags_only)
{
    StatusRegister old_spsr;

    // get spsr_<mode>
    switch (GetMode())
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
        old_spsr.flags.n = new_spsr.flags.n;
        old_spsr.flags.z = new_spsr.flags.z;
        old_spsr.flags.c = new_spsr.flags.c;
        old_spsr.flags.v = new_spsr.flags.v;

        // set updated spsr_<mode>
        switch (GetMode())
        {
            case Mode::FIQ: registers.spsr_fiq = old_spsr; break;
            case Mode::SVC: registers.spsr_svc = old_spsr; break;
            case Mode::ABT: registers.spsr_abt = old_spsr; break;
            case Mode::IRQ: registers.spsr_irq = old_spsr; break;
            case Mode::UND: registers.spsr_und = old_spsr; break;
            case Mode::SYS: LOG("SYS in SPSR flags\n");
        }
        
        return;
    }

    // set updated spsr_<mode>
    switch (GetMode())
    {
        case Mode::FIQ: registers.spsr_fiq = new_spsr; break;
        case Mode::SVC: registers.spsr_svc = new_spsr; break;
        case Mode::ABT: registers.spsr_abt = new_spsr; break;
        case Mode::IRQ: registers.spsr_irq = new_spsr; break;
        case Mode::UND: registers.spsr_und = new_spsr; break;
        case Mode::SYS: LOG("SYS in SPSR\n");    break;
    }
}

// advances the cpu clock
// address is the current access address
// type is the cycle type, either 'n', 's', or 'i'
void Arm7Tdmi::Tick(u8 n, u8 s, u8 i)
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

void Arm7Tdmi::HandleInterrupt()
{
    // exit interrupt
    if (in_interrupt && GetRegister(r15) == 0x138)
    {
        //std::cout << "Handled interrupt!\n";

        // restore registers from stack
        // ldmfd r13! r0-r3, r12, r14
        u32 sp = GetRegister(r13);
        SetRegister(r0,  mem->Read32(sp)); sp += 4;
        SetRegister(r1,  mem->Read32(sp)); sp += 4;
        SetRegister(r2,  mem->Read32(sp)); sp += 4;
        SetRegister(r3,  mem->Read32(sp)); sp += 4;
        SetRegister(r12, mem->Read32(sp)); sp += 4;
        SetRegister(r14, mem->Read32(sp)); sp += 4;
        SetRegister(r13, sp);

        // return from IRQ
        // subs r15, r14, 4
        SetRegister(r15, GetRegister(r14) - 4);

        // restore CPSR
        SetRegister(cpsr, GetRegister(spsr));

        // re-enable interrupts
        registers.cpsr.flags.i = 0;
        mem->Write32Unsafe(REG_IME, 1);

        pipeline_full = false;
        in_interrupt  = false;
        
        //set_state(SYS);

        // clear bit from REG_IF to show that interrupt has been serviced
        u32 reg_if = mem->Read32Unsafe(REG_IF) & ~current_interrupt;
        mem->Write32Unsafe(REG_IF, reg_if);

        //std::cout << "interrupt handled! " << std::hex << registers.r15 << "\n";
        return;
    }

    // check if master interrupts are enabled
    if ((mem->Read32Unsafe(REG_IME) & 1) && registers.cpsr.flags.i == 0) 
    {
        // get enabled interrupts and requested interrupts
        u16 interrupts_enabled   = mem->Read16Unsafe(REG_IE);
        u16 interrupts_requested = mem->Read16Unsafe(REG_IF);

        // get first identical set bit in enabled/requested interrupts
        for (int i = 0; i < 14; ++i) // 14 interrupts available
        {
            // handle interrupt at position i
            if (interrupts_enabled & (1 << i) && interrupts_requested & (1 << i))
            {
                // emulate how BIOS handles interrupts
                //std::cout << "interrupt handling! " << i << "\n";

                u32 old_cpsr = GetRegister(cpsr);
                // switch to IRQ
                SetMode(Mode::IRQ);

                // save CPSR to SPSR
                UpdateSPSR(old_cpsr, false);
                
                // no branch
                if (pipeline_full)
                {
                    if (GetState() == State::ARM) {
                        //std::cout << "arm interrupt\n";
                        SetRegister(r14, GetRegister(r15) - 4);
                    }
                    else {
                        //std::cout << "thumb interrupt\n";
                        SetRegister(r14, GetRegister(r15));
                    }
                }

                // branch
                else
                {
                    //std::cout << "Caution: interrupt after a branch\n";
                    SetRegister(r14, GetRegister(r15) + 4);
                }

                // save registers to SP_irq
                // stmfd  r13!, r0-r3, r12, r14
                u32 sp = GetRegister(r13);
                sp -= 4; mem->Write32(sp, GetRegister(r14)); 
                sp -= 4; mem->Write32(sp, GetRegister(r12));
                sp -= 4; mem->Write32(sp, GetRegister(r3));
                sp -= 4; mem->Write32(sp, GetRegister(r2));
                sp -= 4; mem->Write32(sp, GetRegister(r1));
                sp -= 4; mem->Write32(sp, GetRegister(r0));
                SetRegister(r13, sp);

                // mov r0, 0x4000000
                SetRegister(r0, 0x4000000);

                // address where BIOS returns from IRQ handler
                SetRegister(r14, 0x138);

                // ldr r15, [r0, -0x4]
                SetRegister(r15, mem->Read32(GetRegister(r0) - 0x4) & ~0x3);

                registers.cpsr.flags.i = 1; // disable interrupts
                SetState(State::ARM);
                pipeline_full = false;
                in_interrupt  = true;
                mem->Write32Unsafe(REG_IME, 0);

                // save the current interrupt so we can clear it after it's been serviced
                current_interrupt = interrupts_requested & (1 << i);
                return;
            }
        }
    }
}

u8 Arm7Tdmi::Read8(u32 address)
{
    // reading from BIOS memory
    if (address <= 0x3FFF && registers.r15 > 0x3FFF)
    {
        LOG(LogLevel::Error, "Invalid read from BIOS u8: 0x{x}\n", last_read_bios);
        // u32 value = last_read_bios;
        
        // switch (address & 0x3)
        // {
        //     case 0: value >>= 0;  break;
        //     case 1: value >>= 8;  break;
        //     case 2: value >>= 16; break;
        //     case 3: value >>= 24; break;
        // }

        // return value & 0xFF;
    }

    if ((address >= 0x4000 && address <= 0x1FFFFFF) || address >= 0x10000000)
    {
        LOG(LogLevel::Warning, "UNUSED U8\n");
        switch (GetState())
        {
            case State::ARM: return mem->Read32(registers.r15);
            case State::THUMB:
                exit(0);
                break;
        }
    }


    // if (!mem_check_read(address))
    // {
    //     std::cout << "mem check u8 failed " << std::hex << address << "\n";
    //     // exit(0);
    //     // return last_read_bios & 0xFF;
    // }

    //if ()

    return mem->Read8(address);
}

/*
 * Reads a halfword from the specified memory address
 * pass true if the halfword is signed, false otherwise
 * This needs to be known for misalignment reasons
 */
u32 Arm7Tdmi::Read16(u32 address, bool sign)
{
    // reading from BIOS memory
    if (address <= 0x3FFF && registers.r15 > 0x3FFF)
    {
        LOG(LogLevel::Error, "Invalid read from BIOS u16: 0x{x}\n", last_read_bios);

        u32 value = last_read_bios;
        switch (address & 0x1)
        {
            case 0: value >>= 0;  break;
            case 1: value >>= 16;  break;
        }

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
        case REG_WININ:
        case REG_WINOUT:
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
            LOG(LogLevel::Error, "Invalid read from BIOS u166\n");
            return 0;
    }

    if ((address >= 0x4000 && address <= 0x1FFFFFF) || address >= 0x10000000)
    {
        std::cout << "UNUSED U16\n";
        
        switch (GetState())
        {
            case State::ARM: return mem->Read32(registers.r15);
            case State::THUMB:
                exit(0);
                break;
        }
    }

    // if (!mem_check_read(address))
    // {
    //     std::cout << "mem check u16 failed " << std::hex << address << "\n";
    //     //return last_read_bios & 0xFFFFFF;
    // }

    u32 data;

    if (sign)
    {
        data = (u32) mem->Read16(address);

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
        data = (u32) mem->Read16(address & ~1);
        // misaligned read - reads from forcibly aligned address "addr AND 1", and does then rotate the data as "ROR 8"
        if ((address & 1) != 0)
            BarrelShift(8, data, 0b11);
    }
    
    return data;    
}

/*
 * Reads a word from the specified memory address
 * pass true if this is a LDR or SWP operation false otherwise
 * This needs to be known for misalignment reasons
 */
u32 Arm7Tdmi::Read32(u32 address, bool ldr)
{
    // reading from BIOS memory
    if (address <= 0x3FFF)
    {

        if (registers.r15 < 0x3FFF)
            last_read_bios = mem->Read32Unsafe(address);
        
        return last_read_bios;
    }

    switch (address)
    {
        // case REG_BG0HOFS:
        // case REG_BG1HOFS:
        // case REG_BG2HOFS:
        // case REG_BG3HOFS:
        // case REG_BG0VOFS:
        // case REG_BG1VOFS:
        // case REG_BG2VOFS:
        // case REG_BG3VOFS:
        // case REG_BG2X:
        // case REG_BG2Y:
        // case REG_BG2PA:
        // case REG_BG2PB:
        // case REG_BG2PC:
        // case REG_BG2PD:
        // case REG_BG3X:
        // case REG_BG3Y:
        // case REG_BG3PA:
        // case REG_BG3PB:
        // case REG_BG3PC:
        // case REG_BG3PD:
        // case REG_WIN0H:
        // case REG_WIN1H:
        // case REG_WIN0V:
        // case REG_WIN1V:
        // case REG_WININ:
        // case REG_WINOUT:
        // case REG_MOSAIC:
        // case REG_DMA0SAD:
        // case REG_DMA0DAD:
        case REG_DMA0CNT:
        // case REG_DMA1SAD:
        // case REG_DMA1DAD:
        case REG_DMA1CNT:
        // case REG_DMA2SAD:
        // case REG_DMA2DAD:
        case REG_DMA2CNT:
        // case REG_DMA3SAD:
        // case REG_DMA3DAD:
        case REG_DMA3CNT:
            std::cout << "u32 sadkjflsadfkjsdaflkj\n";
            //return 0;
        default:
            break;
    }

    if ((address >= 0x4000 && address <= 0x1FFFFFF) || address >= 0x10000000)
    {
        std::cout << "UNUSED U32\n";
        switch (GetState())
        {
            
            case State::ARM: return mem->Read32(registers.r15);
            case State::THUMB:
                exit(0);
                break;
        }
    }

    // if (!mem_check_read(address))
    // {
    //     std::cout << "mem check u32 failed " << std::hex << address << "\n";
    //     return last_read_bios;
    // }
    
    // read from forcibly aligned address
    u32 data = mem->Read32(address & ~3);

    // misaligned read - reads from forcibly aligned address "addr AND (NOT 3)", and does then rotate the data as "ROR (addr AND 3)*8"
    // only used for LDR and SWP operations, otherwise just use data from forcibly aligned address
    if (ldr && ((address & 3) != 0))
        BarrelShift((address & 3) << 3, data, 0b11);

    // 8 cycles for gamepak rom access, 5 from mem_check and 3 here
    // if (address >= MEM_GAMEPAK_ROM_START && address <= MEM_GAMEPAK_ROM_END)
    //     cycles += 3;

    return data;
}


void Arm7Tdmi::Write8(u32 address, u8 value)
{
    if (!MemCheckWrite(address)) return;

    // byte write to Palette RAM is written in both upper and lower 8 bytes of halfword at address
    if (address >= MEM_PALETTE_RAM_START && address <= MEM_PALETTE_RAM_END)
    {
        mem->Write8(address, value);
        mem->Write8(address + 1, value);
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
                mem->Write8(address, value);
                mem->Write8(address + 1, value);

                break;

            // bitmap modes
            case 3:
            case 4:
            case 5:
                // writes to OBJ (0x6014000-0x6017FFF) are ignored
                if (address >= 0x6014000)
                    return;
                
                // writes to BG (0x6000000-0x6013FFF) write to both upper and lower bits
                mem->Write8(address, value);
                mem->Write8(address + 1, value);

                break;
        }

        return;
    }

    // normal byte write
    mem->Write8(address, value);
}

void Arm7Tdmi::Write16(u32 address, u16 value)
{
    // align address to halfword
    address &= ~0x1;

    if (!MemCheckWrite(address)) return;
    mem->Write16(address, value);
}

void Arm7Tdmi::Write32(u32 address, u32 value)
{
    // align address to word
    address &= ~0x3;

    if (!MemCheckWrite(address)) return;

    // 8 cycles for gamepak rom access, 5 from mem_check and 3 here
    // if (address >= MEM_GAMEPAK_ROM_START && address <= MEM_GAMEPAK_ROM_END)
    //     cycles += 3;
    mem->Write32(address, value);
}

// determine if a read at the specified address is allowed
inline bool Arm7Tdmi::MemCheckRead(u32 &address)
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

// determine if a write at the specified address is allowed
inline bool Arm7Tdmi::MemCheckWrite(u32 &address)
{
    // upper 4 bits of address bus are unused, so mirror it if trying to access
    if (address >= 0x10000000)
        address &= 0x0FFFFFFF;

    // add cycles for expensive memory accesses

    // +1 cycles for VRAM accress while not in v-blank
    if (address >= MEM_PALETTE_RAM_START && address <= MEM_OAM_END && !mem->stat->displaystat.in_vBlank)
        cycles++;
    
    // bios write
    if (address <= 0x3FFF)
        return false;
    
    return true;
}

bool Arm7Tdmi::CheckState()
{
    bool valid = false;

    switch (GetMode())
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

// #include "HandlerArm.cpp"
// #include "HandlerThumb.cpp"
// #include "swi.cpp"