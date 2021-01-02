/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: arm_7tdmi.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of arm7tdmi functions
 */
#include <iostream>
#include <iomanip>

#include "arm_7tdmi.h"

// uncomment this if running tests
//#define TEST

//#define PRINT

arm_7tdmi::arm_7tdmi()
{
    registers = {0}; // zero out registers
    registers.r15 = 0x8000000; // starting address of gamepak flash rom

    registers.r13     = 0x3007F00; // starting address of user stack
    registers.r13_svc = 0x3007FE0; // starting address of swi stack
    registers.r13_irq = 0x3007FA0; // starting address of interrupt stack

    set_mode(Mode::SVC);
    set_state(State::ARM);

    // initialize cpsr
    registers.cpsr.flags.f = 1;
    registers.cpsr.flags.i = 1;

    pipeline_full = false;
    cycles = 0;
    current_interrupt = 0;
    in_interrupt  = false;
    swi_vblank_intr = false;
    last_read_bios = 0xE129F000;
    mem = NULL;
    
    // different initialization for the testing environment
    #ifdef TEST
    registers.r15 = 0;
    set_state(USR);
    mem = new Memory();
    #endif
}

arm_7tdmi::~arm_7tdmi() { }

Mode arm_7tdmi::get_mode()
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
            std::cout << "Undefined mode\n";
            exit(21);
    } 
}

void arm_7tdmi::set_mode(Mode mode)
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

u8 arm_7tdmi::get_condition_code_flag(ConditionFlag flag)
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

void arm_7tdmi::set_condition_code_flag(ConditionFlag flag, u8 bit)
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
bool arm_7tdmi::condition_met(Condition condition)
{
    switch (condition)
    {
        case Condition::EQ: return get_condition_code_flag(ConditionFlag::Z);  // Z set
        case Condition::NE: return !get_condition_code_flag(ConditionFlag::Z); // Z clear
        case Condition::CS: return get_condition_code_flag(ConditionFlag::C);  // C set
        case Condition::CC: return !get_condition_code_flag(ConditionFlag::C); // C clear
        case Condition::MI: return get_condition_code_flag(ConditionFlag::N);  // N set
        case Condition::PL: return !get_condition_code_flag(ConditionFlag::N); // N Clear
        case Condition::VS: return get_condition_code_flag(ConditionFlag::V);  // V set
        case Condition::VC: return !get_condition_code_flag(ConditionFlag::V); // V clear
        case Condition::HI: return get_condition_code_flag(ConditionFlag::C) && !get_condition_code_flag(ConditionFlag::Z); // C set and Z clear
        case Condition::LS: return !get_condition_code_flag(ConditionFlag::C) || get_condition_code_flag(ConditionFlag::Z); // C clear or Z set
        case Condition::GE: return get_condition_code_flag(ConditionFlag::N) == get_condition_code_flag(ConditionFlag::V);  // N equals V
        case Condition::LT: return get_condition_code_flag(ConditionFlag::N) != get_condition_code_flag(ConditionFlag::V);  // N not equal V
        case Condition::GT: return !get_condition_code_flag(ConditionFlag::Z) && (get_condition_code_flag(ConditionFlag::N) == get_condition_code_flag(ConditionFlag::V)); // Z clear AND (N equals V)
        case Condition::LE: return get_condition_code_flag(ConditionFlag::Z) || (get_condition_code_flag(ConditionFlag::N) != get_condition_code_flag(ConditionFlag::V));  // Z set OR (N not equal to V)
        case Condition::AL: return true; // always
        default: // should never happen
            std::cerr << "Unrecognized condition field\n";
            return false;
    }
}

void arm_7tdmi::fetch()
{
    if (!pipeline_full)
    {
        // fill pipeline
        switch (get_state())
        {
            case State::ARM:
                pipeline[0] = read_u32(registers.r15, false); registers.r15 += 4;
                pipeline[1] = read_u32(registers.r15, false); registers.r15 += 4;
                pipeline[2] = read_u32(registers.r15, false);
                break;
            case State::THUMB:
                pipeline[0] = read_u16(registers.r15, false); registers.r15 += 2;
                pipeline[1] = read_u16(registers.r15, false); registers.r15 += 2;
                pipeline[2] = read_u16(registers.r15, false);
                break;
        }

        pipeline_full = true;
        return;
    }

    switch (get_state())
    {
        case State::ARM:
            pipeline[2] = read_u32(registers.r15, false);
            break;
        case State::THUMB:
            pipeline[2] = (u16) read_u16(registers.r15, false);
            break;
    }
}

void arm_7tdmi::decode(u32 instruction) { }

void arm_7tdmi::execute(u32 instruction)
{  
    #ifdef PRINT
    std::cout << "Executing: " << std::hex << instruction << "\n";
    if (instruction == 0)
        exit(5);
    #endif
    
    switch (get_state())
    {
        case State::ARM:
            if (!condition_met((Condition) util::bitseq<31, 28>(instruction)))
            {
                increment_pc();
                cycle(0, 0, 1); // 1I
                return;
            }
            
            switch(util::get_instruction_format(instruction))
            {
                case ArmInstruction::BEX:  branch_exchange(instruction);        break;
                case ArmInstruction::B:    branch_link(instruction);            break;
                case ArmInstruction::DP:   data_processing(instruction);        break;
                case ArmInstruction::MUL:  multiply(instruction);               break;
                case ArmInstruction::MULL: multiply_long(instruction);          break;
                case ArmInstruction::PSR:  psr_transfer(instruction);           break;
                case ArmInstruction::SDT:  single_data_transfer(instruction);   break;
                case ArmInstruction::HDT:  halfword_data_transfer(instruction); break;
                case ArmInstruction::BDT:  block_data_transfer(instruction);    break;
                case ArmInstruction::SWP:  single_data_swap(instruction);       break;
                case ArmInstruction::INT:  software_interrupt(instruction);     break;
                default:
                    std::cerr << "Cannot execute instruction: " << instruction << "\n";
                    std::cout << registers.r15 << "\n";
                    registers.r15 &= ~0x3;
            }
            break;

        case State::THUMB:
            u16 instr = (u16) instruction;
            switch(util::get_instruction_format((u16) instruction))
            {
                case ThumbInstruction::MSR:    move_shifted_register(instr);            break;
                case ThumbInstruction::ADDSUB: add_sub(instr);                          break;
                case ThumbInstruction::IMM:    move_immediate(instr);                   break;
                case ThumbInstruction::ALU:    alu_thumb(instr);                        break;
                case ThumbInstruction::HI:     hi_reg_ops(instr);                       break;
                case ThumbInstruction::PC:     pc_rel_load(instr);                      break;
                case ThumbInstruction::MOV:    load_store_reg(instr);                   break;
                case ThumbInstruction::MOVS:   load_store_signed_halfword(instr);       break;
                case ThumbInstruction::MOVI:   load_store_immediate(instr);             break;
                case ThumbInstruction::MOVH:   load_store_halfword(instr);              break;
                case ThumbInstruction::SP:     sp_load_store(instr);                    break;
                case ThumbInstruction::LDA:    load_address(instr);                     break;
                case ThumbInstruction::ADDSP:  add_offset_to_sp(instr);                 break;
                case ThumbInstruction::POP:    push_pop(instr);                         break;
                case ThumbInstruction::MOVM:   multiple_load_store(instr);              break;
                case ThumbInstruction::B:      conditional_branch(instr);               break;
                case ThumbInstruction::SWI:    software_interrupt_thumb(instr);         break;
                case ThumbInstruction::BAL:    unconditional_branch(instr);             break;
                case ThumbInstruction::BL:     long_branch_link(instr);                 break;
                default:
                    std::cerr << "Cannot execute thumb instruction: " << (u16) instruction << " " << std::hex << registers.r15 << "\n";
                    registers.r15 &= ~0x1;
            }
            break;
    }

    // increment pc if there was no branch
    if (pipeline_full)
        increment_pc();

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

u32 arm_7tdmi::get_register(u32 reg)
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
            switch (get_mode())
            {
                case Mode::FIQ: return registers.r8_fiq;
                default: return registers.r8;
            }
        case r9:
            switch (get_mode())
            {
                case Mode::FIQ: return registers.r9_fiq;
                default: return registers.r9;
            }
        case r10:
            switch (get_mode())
            {
                case Mode::FIQ: return registers.r10_fiq;
                default: return registers.r10;
            }
        case r11:
            switch (get_mode())
            {
                case Mode::FIQ: return registers.r11_fiq;
                default: return registers.r11;
            }
        case r12:
            switch (get_mode())
            {
                case Mode::FIQ: return registers.r12_fiq;
                default: return registers.r12;
            }

        case r13:
            switch(get_mode())
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
            switch(get_mode())
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
            switch(get_mode())
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

void arm_7tdmi::set_register(u32 reg, u32 val)
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
            switch (get_mode())
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
            switch (get_mode())
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
            switch (get_mode())
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
            switch (get_mode())
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
            switch (get_mode())
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
            switch(get_mode())
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
            switch(get_mode())
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
void arm_7tdmi::update_flags_logical(u32 result, u8 carry_out)
{
    // C flag will be set to the carry out from the barrel shifter
    set_condition_code_flag(ConditionFlag::C, carry_out);

    // Z flag will be set if and only if the result is all zeros
    uint8_t new_z = result == 0 ? 1 : 0;
    set_condition_code_flag(ConditionFlag::Z, new_z);

    // N flag will be set to the logical value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    set_condition_code_flag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary
}

// update cpsr flags after an addition operation
void arm_7tdmi::update_flags_addition(u32 op1, u32 op2, u32 result)
{
    // C flag will be set to the carry out of bit 31 of the ALU
    if (op1 > result || op2 > result)
        set_condition_code_flag(ConditionFlag::C, 1);
    else
        set_condition_code_flag(ConditionFlag::C, 0);

    // Z flag will be set if and only if the result was zero
    u8 new_z = result == 0 ? 1 : 0;
    set_condition_code_flag(ConditionFlag::Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    u8 new_n = result & 0x80000000 ? 1 : 0;
    set_condition_code_flag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    u8 op1_msb = op1 & 0x80000000 ? 1 : 0;
    u8 op2_msb = op2 & 0x80000000 ? 1 : 0;
    u8 result_msb = result & 0x80000000 ? 1 : 0;

    if (op1_msb == 0 && op2_msb == 0 && result_msb == 1)
        set_condition_code_flag(ConditionFlag::V, 1);
    else if (op1_msb == 1 && op2_msb == 1 && result_msb == 0)
        set_condition_code_flag(ConditionFlag::V, 1);
    else
        set_condition_code_flag(ConditionFlag::V, 0);
}

// update cpsr flags after a subtraction operation
void arm_7tdmi::update_flags_subtraction(u32 op1, u32 op2, u32 result)
{
    // C flag will be set to the carry out of bit 31 of the ALU
    // ARM uses an inverted carry flag for borrow
    if (result > op1)
        set_condition_code_flag(ConditionFlag::C, 0);
    else
        set_condition_code_flag(ConditionFlag::C, 1);

    // Z flag will be set if and only if the result was zero
    uint8_t new_z = result == 0 ? 1 : 0;
    set_condition_code_flag(ConditionFlag::Z, new_z);

    // N flag will be set to the value of bit 31 of the result
    uint8_t new_n = result & 0x80000000 ? 1 : 0;
    set_condition_code_flag(ConditionFlag::N, new_n); // 0x80000000 is 1 followed by 31 zeros in binary

    // V flag will be set overflow occurs into bit 31 of the result
    u8 op1_msb = op1 & 0x80000000 ? 1 : 0;
    u8 op2_msb = op2 & 0x80000000 ? 1 : 0;
    u8 result_msb = result & 0x80000000 ? 1 : 0;

    if (op1_msb == 0 && op2_msb == 1 && result_msb == 1)
        set_condition_code_flag(ConditionFlag::V, 1);
    else if (op1_msb == 1 && op2_msb == 0 && result_msb == 0)
        set_condition_code_flag(ConditionFlag::V, 1);
    else
        set_condition_code_flag(ConditionFlag::V, 0);

}

/* performs a shift operation on op2.
 * returns the carry-out of the barrel shifter
 *
 *  @params:
 *  shift_amount - the amount of times to shift
 *  num - the number that will actually be shifted
 *  opcode - an encoding of which type of shift to be performed
 */
u8 arm_7tdmi::barrel_shift(u32 shift_amount, u32 &num, u8 opcode)
{
    u8 carry_out = get_condition_code_flag(ConditionFlag::C); // preserve C flag

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
                num |= get_condition_code_flag(ConditionFlag::C) << (num_bits - 1);
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

inline void arm_7tdmi::increment_pc()
{
    registers.r15 += get_state() == State::ARM ? 4 : 2;
}

/*
 * Updates the value in the cpsr
 * Can also change the emulator's state or mode depending on the value
 */ 
void arm_7tdmi::update_cpsr(u32 value, bool flags_only)
{
    status_register sr;
    sr.raw = value;

    // in user mode, only condition bits can be changed
    if (flags_only || get_mode() == Mode::USR)
    {
        registers.cpsr.flags.n = sr.flags.n;
        registers.cpsr.flags.z = sr.flags.z;
        registers.cpsr.flags.c = sr.flags.c;
        registers.cpsr.flags.v = sr.flags.v;
        return;
    }

    registers.cpsr.raw = value;

    if (registers.cpsr.flags.t != sr.flags.t)
        std::cout << "Software is changing TBIT in CPSR!" << "\n"; // is this allowed??

    // TODO - validate CPSR was appropriately changed
    bool valid = false;

    switch (get_mode())
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
void arm_7tdmi::update_spsr(u32 value, bool flags_only)
{
    status_register old_spsr;

    // get spsr_<mode>
    switch (get_mode())
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
    status_register new_spsr;
    new_spsr.raw = value;

    // don't have to check for USR mode b/c that was done above
    if (flags_only)
    {
        old_spsr.flags.n = new_spsr.flags.n;
        old_spsr.flags.z = new_spsr.flags.z;
        old_spsr.flags.c = new_spsr.flags.c;
        old_spsr.flags.v = new_spsr.flags.v;

        // set updated spsr_<mode>
        switch (get_mode())
        {
            case Mode::FIQ: registers.spsr_fiq = old_spsr; break;
            case Mode::SVC: registers.spsr_svc = old_spsr; break;
            case Mode::ABT: registers.spsr_abt = old_spsr; break;
            case Mode::IRQ: registers.spsr_irq = old_spsr; break;
            case Mode::UND: registers.spsr_und = old_spsr; break;
            case Mode::SYS: std::cout << "SYS in SPSR flags\n";  break;
        }
        
        return;
    }

    // set updated spsr_<mode>
    switch (get_mode())
    {
        case Mode::FIQ: registers.spsr_fiq = new_spsr; break;
        case Mode::SVC: registers.spsr_svc = new_spsr; break;
        case Mode::ABT: registers.spsr_abt = new_spsr; break;
        case Mode::IRQ: registers.spsr_irq = new_spsr; break;
        case Mode::UND: registers.spsr_und = new_spsr; break;
        case Mode::SYS: std::cout << "SYS in SPSR\n";  break;
    }
}

// advances the cpu clock
// address is the current access address
// type is the cycle type, either 'n', 's', or 'i'
void arm_7tdmi::cycle(u8 n, u8 s, u8 i)
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

void arm_7tdmi::handle_interrupt()
{
    // exit interrupt
    if (in_interrupt && get_register(r15) == 0x138)
    {
        //std::cout << "Handled interrupt!\n";

        // restore registers from stack
        // ldmfd r13! r0-r3, r12, r14
        u32 sp = get_register(r13);
        set_register(r0,  mem->read_u32(sp)); sp += 4;
        set_register(r1,  mem->read_u32(sp)); sp += 4;
        set_register(r2,  mem->read_u32(sp)); sp += 4;
        set_register(r3,  mem->read_u32(sp)); sp += 4;
        set_register(r12, mem->read_u32(sp)); sp += 4;
        set_register(r14, mem->read_u32(sp)); sp += 4;
        set_register(r13, sp);

        // return from IRQ
        // subs r15, r14, 4
        set_register(r15, get_register(r14) - 4);

        // restore CPSR
        set_register(cpsr, get_register(spsr));

        // re-enable interrupts
        registers.cpsr.flags.i = 0;
        mem->write_u32_unprotected(REG_IME, 1);

        pipeline_full = false;
        in_interrupt  = false;
        
        //set_state(SYS);

        // clear bit from REG_IF to show that interrupt has been serviced
        u32 reg_if = mem->read_u32_unprotected(REG_IF) & ~current_interrupt;
        mem->write_u32_unprotected(REG_IF, reg_if);

        //std::cout << "interrupt handled! " << std::hex << registers.r15 << "\n";
        return;
    }

    // check if master interrupts are enabled
    if ((mem->read_u32_unprotected(REG_IME) & 1) && registers.cpsr.flags.i == 0) 
    {
        // get enabled interrupts and requested interrupts
        u16 interrupts_enabled   = mem->read_u16_unprotected(REG_IE);
        u16 interrupts_requested = mem->read_u16_unprotected(REG_IF);

        // get first identical set bit in enabled/requested interrupts
        for (int i = 0; i < 14; ++i) // 14 interrupts available
        {
            // handle interrupt at position i
            if (interrupts_enabled & (1 << i) && interrupts_requested & (1 << i))
            {
                // emulate how BIOS handles interrupts
                //std::cout << "interrupt handling! " << i << "\n";

                u32 old_cpsr = get_register(cpsr);
                // switch to IRQ
                set_mode(Mode::IRQ);

                // save CPSR to SPSR
                update_spsr(old_cpsr, false);
                
                // no branch
                if (pipeline_full)
                {
                    if (get_state() == State::ARM) {
                        //std::cout << "arm interrupt\n";
                        set_register(r14, get_register(r15) - 4);
                    }
                    else {
                        //std::cout << "thumb interrupt\n";
                        set_register(r14, get_register(r15));
                    }
                }

                // branch
                else
                {
                    //std::cout << "Caution: interrupt after a branch\n";
                    set_register(r14, get_register(r15) + 4);
                }

                // save registers to SP_irq
                // stmfd  r13!, r0-r3, r12, r14
                u32 sp = get_register(r13);
                sp -= 4; mem->write_u32(sp, get_register(r14)); 
                sp -= 4; mem->write_u32(sp, get_register(r12));
                sp -= 4; mem->write_u32(sp, get_register(r3));
                sp -= 4; mem->write_u32(sp, get_register(r2));
                sp -= 4; mem->write_u32(sp, get_register(r1));
                sp -= 4; mem->write_u32(sp, get_register(r0));
                set_register(r13, sp);

                // mov r0, 0x4000000
                set_register(r0, 0x4000000);

                // address where BIOS returns from IRQ handler
                set_register(r14, 0x138);

                // ldr r15, [r0, -0x4]
                set_register(r15, mem->read_u32(get_register(r0) - 0x4) & ~0x3);

                registers.cpsr.flags.i = 1; // disable interrupts
                set_state(State::ARM);
                pipeline_full = false;
                in_interrupt  = true;
                mem->write_u32_unprotected(REG_IME, 0);

                // save the current interrupt so we can clear it after it's been serviced
                current_interrupt = interrupts_requested & (1 << i);
                return;
            }
        }
    }
}

u8 arm_7tdmi::read_u8(u32 address)
{
    // reading from BIOS memory
    if (address <= 0x3FFF && registers.r15 > 0x3FFF)
    {
        std::cout << "Invalid read from BIOS u8: " << std::hex << last_read_bios << "\n";
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
        std::cout << "UNUSED U8\n";
        switch (get_state())
        {
            case State::ARM: return mem->read_u32(registers.r15);
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

    return mem->read_u8(address);
}

/*
 * Reads a halfword from the specified memory address
 * pass true if the halfword is signed, false otherwise
 * This needs to be known for misalignment reasons
 */
u32 arm_7tdmi::read_u16(u32 address, bool sign)
{
    // reading from BIOS memory
    if (address <= 0x3FFF && registers.r15 > 0x3FFF)
    {
        std::cout << "Invalid read from BIOS u16: " << std::hex << last_read_bios << "\n";
        //exit(0);
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
            std::cout << "u16 sadkjflsadfkjsdaflkj\n";
            return 0;
    }

    if ((address >= 0x4000 && address <= 0x1FFFFFF) || address >= 0x10000000)
    {
        std::cout << "UNUSED U16\n";
        
        switch (get_state())
        {
            case State::ARM: return mem->read_u32(registers.r15);
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
        data = (u32) mem->read_u16(address);

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
        data = (u32) mem->read_u16(address & ~1);
        // misaligned read - reads from forcibly aligned address "addr AND 1", and does then rotate the data as "ROR 8"
        if ((address & 1) != 0)
            barrel_shift(8, data, 0b11);
    }
    
    return data;    
}

/*
 * Reads a word from the specified memory address
 * pass true if this is a LDR or SWP operation false otherwise
 * This needs to be known for misalignment reasons
 */
u32 arm_7tdmi::read_u32(u32 address, bool ldr)
{
    // reading from BIOS memory
    if (address <= 0x3FFF)
    {

        if (registers.r15 < 0x3FFF)
            last_read_bios = mem->read_u32_unprotected(address);
        
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
        switch (get_state())
        {
            
            case State::ARM: return mem->read_u32(registers.r15);
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
    u32 data = mem->read_u32(address & ~3);

    // misaligned read - reads from forcibly aligned address "addr AND (NOT 3)", and does then rotate the data as "ROR (addr AND 3)*8"
    // only used for LDR and SWP operations, otherwise just use data from forcibly aligned address
    if (ldr && ((address & 3) != 0))
        barrel_shift((address & 3) << 3, data, 0b11);

    // 8 cycles for gamepak rom access, 5 from mem_check and 3 here
    // if (address >= MEM_GAMEPAK_ROM_START && address <= MEM_GAMEPAK_ROM_END)
    //     cycles += 3;

    return data;
}


void arm_7tdmi::write_u8(u32 address, u8 value)
{
    if (!mem_check_write(address)) return;

    // byte write to Palette RAM is written in both upper and lower 8 bytes of halfword at address
    if (address >= MEM_PALETTE_RAM_START && address <= MEM_PALETTE_RAM_END)
    {
        mem->write_u8(address, value);
        mem->write_u8(address + 1, value);
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
                mem->write_u8(address, value);
                mem->write_u8(address + 1, value);

                break;

            // bitmap modes
            case 3:
            case 4:
            case 5:
                // writes to OBJ (0x6014000-0x6017FFF) are ignored
                if (address >= 0x6014000)
                    return;
                
                // writes to BG (0x6000000-0x6013FFF) write to both upper and lower bits
                mem->write_u8(address, value);
                mem->write_u8(address + 1, value);

                break;
        }

        return;
    }

    // normal byte write
    mem->write_u8(address, value);
}

void arm_7tdmi::write_u16(u32 address, u16 value)
{
    // align address to halfword
    address &= ~0x1;

    if (!mem_check_write(address)) return;
    mem->write_u16(address, value);
}

void arm_7tdmi::write_u32(u32 address, u32 value)
{
    // align address to word
    address &= ~0x3;

    if (!mem_check_write(address)) return;

    // 8 cycles for gamepak rom access, 5 from mem_check and 3 here
    // if (address >= MEM_GAMEPAK_ROM_START && address <= MEM_GAMEPAK_ROM_END)
    //     cycles += 3;
    mem->write_u32(address, value);
}

// determine if a read at the specified address is allowed
inline bool arm_7tdmi::mem_check_read(u32 &address)
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
inline bool arm_7tdmi::mem_check_write(u32 &address)
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

bool arm_7tdmi::check_state()
{
    bool valid = false;

    switch (get_mode())
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

// #include "arm_alu.cpp"
// #include "thumb_alu.cpp"
// #include "swi.cpp"