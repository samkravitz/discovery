/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: util.h
 * DATE: June 19, 2020
 * DESCRIPTION: utility functions that don't belong in any class
 */

#pragma once

#include <string>
#include "common.h"

using std::string;

namespace util
{
    // determine which type of operation the instruction is
    ArmInstruction getInstructionFormat(u32);

    // determine which type of thumb operation an instruction is
    ThumbInstruction getInstructionFormat(u16);

    // test if path exists
    bool pathExists(string const &);

    /**
     * get subset of bits for purposes like destination register, opcode, shifts
     * All instructions have data hidden within their codes;
     * ex: A branch instruction holds the offset in bits [23-0]
     * This function will extract the bits.
     * Since the reference I am using is in reverse bit order, end >= start must be true
     * 
     * ex: bitseq<7, 4>(0b11110000) = 0b1111
     */
    template <int end, int start>
    inline u32 bitseq(u32);

    template <int end, int start>
    inline u16 bitseq(u16);

    inline s8 signum(double);
};

// util inline functions
#include "util.inl"
