/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: util.cpp
 * DATE: June 27, 2020
 * DESCRIPTION: utility function implementations
 */

#include <iostream>
#include <bitset>
#include "util.h"

/* get subset of instruction for purposes like destination register, opcode, shifts
 * All instructions have data hidden within their codes;
 * ex: A branch instruction holds the offset in bits [23-0]
 * This function will extract the bits.
 * Since the reference I am using is in reverse bit order, end >= start must be true
 * 
 * ex: getInstructionSubset(0b11110000, 7, 4) = 0b1111
 */
uint32_t util::get_instruction_subset(arm_instruction instruction, int end, int start) {
    if (end < start) return 0;

    std::bitset<32> instr(instruction);
    uint32_t subset = 0;

    for (int i = end; i >= start; --i) {
        subset <<= 1;
        subset |= instr[i];
    }

    return subset;
}

// determine if the condition field of an instruction is true, given the state of the CPSR
bool util::condition_met(arm_instruction instruction, arm_7tdmi &cpu) {
    // get condition field from instruction
    condition_t condition_field = (condition_t) get_instruction_subset(instruction, 31, 28);
    switch (condition_field) {
        case EQ: return cpu.get_condition_code_flag(Z); // Z set
        case NE: return !cpu.get_condition_code_flag(Z); // Z clear
        case CS: return cpu.get_condition_code_flag(C); // C set
        case CC: return !cpu.get_condition_code_flag(C); // C clear
        case MI: return cpu.get_condition_code_flag(N); // N set
        case PL: return !cpu.get_condition_code_flag(N); // N Clear
        case VS: return cpu.get_condition_code_flag(V); // V set
        case VC: return !cpu.get_condition_code_flag(V); // V clear
        case HI: return cpu.get_condition_code_flag(C) && !cpu.get_condition_code_flag(Z); // C set and Z clear
        case LS: return !cpu.get_condition_code_flag(C) || cpu.get_condition_code_flag(Z); // C clear or Z set
        case GE: return cpu.get_condition_code_flag(N) == cpu.get_condition_code_flag(V); // N equals V
        case LT: return cpu.get_condition_code_flag(N) != cpu.get_condition_code_flag(V); // N not equal V
        case GT: return !cpu.get_condition_code_flag(Z) && (cpu.get_condition_code_flag(N) == cpu.get_condition_code_flag(V)); // Z clear AND (N equals V)
        case LE: return cpu.get_condition_code_flag(Z) || (cpu.get_condition_code_flag(N) != cpu.get_condition_code_flag(V)); // Z set OR (N not equal to V)
        case AL: return true; // always
        default: // should never happen
            std::cerr << "Unrecognized condition field: " << instruction << "\n";
            return false;
    }
}

// determine which type of operation the instruction is
// see docs/arm_instruction_set_bitfield.png to see a visual of the different types of instructions
// basically each instruction has its own required bits that need to be set, this function just looks for those bits
// a lot of this code is taken from shonumi's GBE+ (https://github.com/shonumi/gbe-plus/blob/master/src/gba/arm7.cpp)
instruction_set_format_t util::get_instruction_format(arm_instruction instruction) {
    if ((instruction >> 4 & 0b111111111111111111111111) == 0b000100101111111111110001) return BEX; // BEX
    else if ((instruction >> 25 & 0b111) == 0b101) return B; // Branch

    else if ((instruction & 0xD900000) == 0x1000000) { // 24th bit is 1;
        if ((instruction & 0x80) && (instruction & 0x10) && ((instruction & 0x2000000) == 0)) { // 7th bit is 1, 4th bit is 1, 25th bit is 0
            if ((instruction >> 5 & 0x3) == 0) return SDS; // bits 5-6 are 00
            else return HDT_IO;
        } 
        else return PSR;
    }

    else if ((instruction >> 26 & 0x3) == 0x0) { // bits 26-27 are 0
        if ((instruction & 0x80) && ((instruction & 0x10) == 0)) { // 7th bit is 1, 4th bit is 0
            if (instruction & 0x2000000) return DP; // 25th bit is 1
            else if ((instruction & 0x100000) && ((instruction >> 23 & 0x3) == 0x2)) return DP; // 20th bit is 1, 24-23th bit is 10
            else if(((instruction >> 23) & 0x3) != 0x2) return DP;
            else return MUL;
        }

        else if ((instruction & 0x80) && (instruction & 0x10)) { // 7th bit is 1, 4th bit is 1
            if ((instruction >> 4 & 0xF) == 0x9) { // bits 7-4 are 1001
                if(instruction & 0x2000000) return DP; // 25th bit is 1
                else if(((instruction >> 23) & 0x3) == 0x2) return SDS; // bits 24-23 are 10
                else return MUL;
            }
            else if (instruction & 0x2000000) return DP;
            else return HDT_IO;
        }

        else return DP;
    }

    else if ((instruction >> 26 & 0x3) == 0x1) return SDT; // bits 27-26 are 01
    else if ((instruction >> 25 & 0x7) == 0x4) return BDT; // bits 27-25 are 100
    else if ((instruction >> 24 & 0xF) == 0xF) return INT; // bits
    else return UNDEF;
}