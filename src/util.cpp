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
#include "../include/util.h"

/* get subset of instruction for purposes like destination register, opcode, shifts
 * All instructions have data hidden within their codes;
 * ex: A branch instruction holds the offset in bits [23-0]
 * This function will extract the bits.
 * Since the reference I am using is in reverse bit order, end >= start must be true
 * 
 * ex: getInstructionSubset(0b11110000, 7, 4) = 0b1111
 */
u32 util::get_instruction_subset(u32 instruction, int end, int start)
{
    if (end < start)
        return 0;

    std::bitset<32> instr(instruction);
    uint32_t subset = 0;

    for (int i = end; i >= start; --i) {
        subset <<= 1;
        subset |= instr[i];
    }

    return subset;
}

u16 util::get_instruction_subset(u16 instruction, int end, int start)
{
    if (end < start)
        return 0;

    std::bitset<32> instr(instruction);
    uint32_t subset = 0;

    for (int i = end; i >= start; --i)
    {
        subset <<= 1;
        subset |= instr[i];
    }

    return subset;
}

// determine which type of operation the instruction is
// see docs/arm_instruction_set_bitfield.png to see a visual of the different types of instructions
// basically each instruction has its own required bits that need to be set, this function just looks for those bits
// a lot of this code is taken from shonumi's GBE+ (https://github.com/shonumi/gbe-plus/blob/master/src/gba/arm7.cpp)
instruction_set_format_t util::get_instruction_format(u32 instruction)
{
    if ((instruction >> 4 & 0b111111111111111111111111) == 0b000100101111111111110001)
        return BEX; // BEX
    else if ((instruction >> 25 & 0b111) == 0b101)
        return B; // Branch

    // 24th bit is 1
    else if ((instruction & 0xD900000) == 0x1000000)
    {
        // 7th bit is 1, 4th bit is 1, 25th bit is 0
        if ((instruction & 0x80) && (instruction & 0x10) && ((instruction & 0x2000000) == 0))
        {
            if ((instruction >> 5 & 0x3) == 0) // bits 5-6 are 00
                return SWP;
            else
                return HDT;
        } 
        else
            return PSR;
    }

    // bits 26-27 are 0
    else if ((instruction >> 26 & 0x3) == 0x0)
    {
        // 7th bit is 1, 4th bit is 0
        if ((instruction & 0x80) && ((instruction & 0x10) == 0))
        {
            if (instruction & 0x2000000) // 25th bit is 1
                return DP;
            else if ((instruction & 0x100000) && ((instruction >> 23 & 0x3) == 0x2))// 20th bit is 1, 24-23th bit is 10
                return DP; 
            else if(((instruction >> 23) & 0x3) != 0x2)
                return DP;
            else if (instruction & 0x800000) // 23rd bit is 1
                return MULL;
            else
                return MUL;
        }

        // 7th bit is 1, 4th bit is 1
        else if ((instruction & 0x80) && (instruction & 0x10))
        {
            // bits 7-4 are 1001
            if ((instruction >> 4 & 0xF) == 0x9)
            {
                if (instruction & 0x2000000) // 25th bit is 1
                    return DP; 
                else if (((instruction >> 23) & 0x3) == 0x2) // bits 24-23 are 10
                    return SWP;
                else if (instruction & 0x800000) // 23rd bit is 1
                    return MULL;
                else
                    return MUL;
            }
            else if (instruction & 0x2000000)
                return DP;
            else
                return HDT;
        }

        else
            return DP;
    }

    else if ((instruction >> 26 & 0x3) == 0x1) // bits 27-26 are 01
        return SDT;
    else if ((instruction >> 25 & 0x7) == 0x4) // bits 27-25 are 100
        return BDT;
    else if ((instruction >> 24 & 0xF) == 0xF)
        return INT;
    else return UNDEF;
}

// determine which type of thumb operation an instruction is
thumb_instruction_format_t util::get_instruction_format(u16 instruction)
{
    if ((instruction >> 13 & 0b111) == 0)
    {
        if ((instruction >> 11 & 0b11) == 0b11)
            return ADDSUB_T;
        else
            return MSR_T;
    }

    else if ((instruction >> 13 & 0b111) == 0b001)
        return IMM_T;

    else if ((instruction >> 10 & 0b111111) == 0b010000)
        return ALU_T;

    else if ((instruction >> 10 & 0b111111) == 0b010001)
        return HI_T;

    else if ((instruction >> 11 & 0b11111) == 0b01001)
        return PC_T;

    else if ((instruction >> 12 & 0b1111) == 0b0101)
    {
        if ((instruction >> 9 & 1) == 0)
            return MOV_T;
        else
            return MOVS_T;
    }
    
    else if ((instruction >> 13 & 0b111) == 0b011)
        return MOVI_T;

    else if ((instruction >> 12 & 0b1111) == 0b1000)
        return MOVH_T;

    else if ((instruction >> 12 & 0b1111) == 0b1001)
        return SP_T;

    else if ((instruction >> 12 & 0b1111) == 0b1010)
        return LDA_T;

    else if ((instruction >> 12 & 0b1111) == 0b1011)
    {
        if ((instruction >> 9 & 0b111) == 0b000)
            return ADDSP_T;
        else
            return POP_T;
    }

    else if ((instruction >> 12 & 0b1111) == 0b1100)
        return MOVM_T;

    else if ((instruction >> 12 & 0b1111) == 0b1101)
    {
        if ((instruction >> 8 & 0b1111) == 0b1111)
            return SWI_T;
        else
            return B_T;
    }

    else if ((instruction >> 11 & 0b11111) == 0b11100)
        return BAL_T;

    else if ((instruction >> 12 & 0b1111) == 0b1111)
        return BL_T;

    else
        return UND_T;
}