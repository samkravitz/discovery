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
instruction_set_format_t util::get_instruction_format(arm_instruction instruction) {
    if ((instruction >> 4 & 0b111111111111111111111111) == 0b000100101111111111110001) return BEX; // BEX
    if ((instruction >> 25 & 0b111) == 0b101) return B; // Branch
    if ((instruction >> 26 & 0b11) == 0b00) return DP; // Data Processing / PSR Transfer
    if (((instruction >> 4 & 0b1111) == 0b1001) && ((instruction >> 22 & 0b111111) == 0b000000)) return MUL; // Multiply
    if (((instruction >> 4 & 0b1111) == 0b1001) && ((instruction >> 23 & 0b11111) == 0b00001)) return MULL; // Multiply Long
    if ((instruction >> 26 & 0b11) == 0b01) return SDT; // Single Data Transfer
    if (((instruction >> 4 & 0b1) == 0b1)
        && ((instruction >> 7 & 0b1) == 0b1)
        && ((instruction >> 22 & 0b1) == 0b1)
        && ((instruction >> 25 & 0b111) == 0b000)) return HDT_IO; // Halfword Data Transfer - immediate offset
    if (((instruction >> 4 & 0b1) == 0b1)
        && ((instruction >> 7 & 0b11111) == 0b00001)
        && ((instruction >> 22 & 0b1) == 0b0)
        && ((instruction >> 25 & 0b111) == 0b000)) return HDT_RO; // Halfword Data Transfer - register offset
    if ((instruction >> 25 & 0b111) == 0b100) return BDT; // Block Data Transfer
    if (((instruction >> 4 & 0b11111111) == 0b00001001)
        && ((instruction >> 20 & 0b11) == 0b00)
        && ((instruction >> 23 & 0b11111) == 0b00010)) return SDS; // Single Data Swap
    if ((instruction >> 24 & 0b1111) == 0b1111) return INT; // Software Interrupt
    if (((instruction >> 4 & 0b1) == 0b0) && ((instruction >> 24 & 0b1111) == 0b1110)) return CDO; // Coprocessor Data Operation
    if ((instruction >> 25 & 0b111) == 0b110) return CDT; // Coprocessor Data Transfer
    if (((instruction >> 4 & 0b1) == 0b1) && ((instruction >> 24 & 0b1111) == 0b1110)) return CRT; // Coprocessor Register Transfer
    if (((instruction >> 4 & 0b1) == 0b1) && ((instruction >> 25 & 0b111) == 0b011)) return UNDEF; // Undefined
    
    // Unknown instruction format
    std::bitset<32> bs(instruction);
    std::cerr << "Unknown Instruction Format: " << bs << "\n";
    return UNKNOWN_INSTRUCTION_FORMAT;
}