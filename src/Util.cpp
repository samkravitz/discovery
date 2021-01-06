/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Ttil.cpp
 * DATE: June 27, 2020
 * DESCRIPTION: utility function implementations
 */

#include "Util.h"

// determine which type of operation the instruction is
// see docs/arm_instruction_set_bitfield.png to see a visual of the different types of instructions
// basically each instruction has its own required bits that need to be set, this function just looks for those bits
// a lot of this code is taken from shonumi's GBE+ (https://github.com/shonumi/gbe-plus/blob/master/src/gba/arm7.cpp)
ArmInstruction Util::GetInstructionFormat(u32 instruction)
{
    if ((instruction >> 4 & 0b111111111111111111111111) == 0b000100101111111111110001)
        return ArmInstruction::BEX; // BEX
    else if ((instruction >> 25 & 0b111) == 0b101)
        return ArmInstruction::B; // Branch

    // 24th bit is 1
    else if ((instruction & 0xD900000) == 0x1000000)
    {
        // 7th bit is 1, 4th bit is 1, 25th bit is 0
        if ((instruction & 0x80) && (instruction & 0x10) && ((instruction & 0x2000000) == 0))
        {
            if ((instruction >> 5 & 0x3) == 0) // bits 5-6 are 00
                return ArmInstruction::SWP;
            else
                return ArmInstruction::HDT;
        } 
        else
            return ArmInstruction::PSR;
    }

    // bits 26-27 are 0
    else if ((instruction >> 26 & 0x3) == 0x0)
    {
        // 7th bit is 1, 4th bit is 0
        if ((instruction & 0x80) && ((instruction & 0x10) == 0))
        {
            if (instruction & 0x2000000) // 25th bit is 1
                return ArmInstruction::DP;
            else if ((instruction & 0x100000) && ((instruction >> 23 & 0x3) == 0x2))// 20th bit is 1, 24-23th bit is 10
                return ArmInstruction::DP; 
            else if(((instruction >> 23) & 0x3) != 0x2)
                return ArmInstruction::DP;
            else if (instruction & 0x800000) // 23rd bit is 1
                return ArmInstruction::MULL;
            else
                return ArmInstruction::MUL;
        }

        // 7th bit is 1, 4th bit is 1
        else if ((instruction & 0x80) && (instruction & 0x10))
        {
            // bits 7-4 are 1001
            if ((instruction >> 4 & 0xF) == 0x9)
            {
                if (instruction & 0x2000000) // 25th bit is 1
                    return ArmInstruction::DP; 
                else if (((instruction >> 23) & 0x3) == 0x2) // bits 24-23 are 10
                    return ArmInstruction::SWP;
                else if (instruction & 0x800000) // 23rd bit is 1
                    return ArmInstruction::MULL;
                else
                    return ArmInstruction::MUL;
            }
            else if (instruction & 0x2000000)
                return ArmInstruction::DP;
            else
                return ArmInstruction::HDT;
        }

        else
            return ArmInstruction::DP;
    }

    else if ((instruction >> 26 & 0x3) == 0x1) // bits 27-26 are 01
        return ArmInstruction::SDT;
    else if ((instruction >> 25 & 0x7) == 0x4) // bits 27-25 are 100
        return ArmInstruction::BDT;
    else if ((instruction >> 24 & 0xF) == 0xF)
        return ArmInstruction::INT;
    else return ArmInstruction::UNDEF;
}

// determine which type of thumb operation an instruction is
ThumbInstruction Util::GetInstructionFormat(u16 instruction)
{
    if ((instruction >> 13 & 0b111) == 0)
    {
        if ((instruction >> 11 & 0b11) == 0b11)
            return ThumbInstruction::ADDSUB;
        else
            return ThumbInstruction::MSR;
    }

    else if ((instruction >> 13 & 0b111) == 0b001)
        return ThumbInstruction::IMM;

    else if ((instruction >> 10 & 0b111111) == 0b010000)
        return ThumbInstruction::ALU;

    else if ((instruction >> 10 & 0b111111) == 0b010001)
        return ThumbInstruction::HI;

    else if ((instruction >> 11 & 0b11111) == 0b01001)
        return ThumbInstruction::PC;

    else if ((instruction >> 12 & 0b1111) == 0b0101)
    {
        if ((instruction >> 9 & 1) == 0)
            return ThumbInstruction::MOV;
        else
            return ThumbInstruction::MOVS;
    }
    
    else if ((instruction >> 13 & 0b111) == 0b011)
        return ThumbInstruction::MOVI;

    else if ((instruction >> 12 & 0b1111) == 0b1000)
        return ThumbInstruction::MOVH;

    else if ((instruction >> 12 & 0b1111) == 0b1001)
        return ThumbInstruction::SP;

    else if ((instruction >> 12 & 0b1111) == 0b1010)
        return ThumbInstruction::LDA;

    else if ((instruction >> 12 & 0b1111) == 0b1011)
    {
        if ((instruction >> 9 & 0b111) == 0b000)
            return ThumbInstruction::ADDSP;
        else
            return ThumbInstruction::POP;
    }

    else if ((instruction >> 12 & 0b1111) == 0b1100)
        return ThumbInstruction::MOVM;

    else if ((instruction >> 12 & 0b1111) == 0b1101)
    {
        if ((instruction >> 8 & 0b1111) == 0b1111)
            return ThumbInstruction::SWI;
        else
            return ThumbInstruction::B;
    }

    else if ((instruction >> 11 & 0b11111) == 0b11100)
        return ThumbInstruction::BAL;

    else if ((instruction >> 12 & 0b1111) == 0b1111)
        return ThumbInstruction::BL;

    else
        return ThumbInstruction::UND;
}