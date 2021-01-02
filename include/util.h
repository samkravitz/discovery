/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: util.h
 * DATE: June 19, 2020
 * DESCRIPTION: utility functions that don't belong in any class
 */

#pragma once

#include <bitset>

#include "common.h"
#include "cpu.h"

namespace util
{
    // determine which type of operation the instruction is
    ArmInstruction get_instruction_format(u32 instruction);

    // determine which type of thumb operation an instruction is
    ThumbInstruction get_instruction_format(u16 instruction);
    

    /* get subset of bits for purposes like destination register, opcode, shifts
    * All instructions have data hidden within their codes;
    * ex: A branch instruction holds the offset in bits [23-0]
    * This function will extract the bits.
    * Since the reference I am using is in reverse bit order, end >= start must be true
    * 
    * ex: bitseq<7, 4>(0b11110000) = 0b1111
    */
    template <int end, int start>
    u32 bitseq(u32 val)
    {
        if (end < start)
            return 0;

        std::bitset<32> bits(val);
        u32 subset = 0;

        for (int i = end; i >= start; --i)
        {
            subset <<= 1;
            subset |= bits[i];
        }

        return subset;
    }

    template <int end, int start>
    u16 bitseq(u16 val)
    {
        if (end < start)
            return 0;

        std::bitset<16> bits(val);
        u16 subset = 0;

        for (int i = end; i >= start; --i)
        {
            subset <<= 1;
            subset |= bits[i];
        }

        return subset;
    }
}