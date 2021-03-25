/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: util.h
 * DATE: June 19, 2020
 * DESCRIPTION: utility functions that don't belong in any class
 */

#pragma once

#include "common.h"
#include <string>

using std::string;

namespace util
{
    // determine which type of operation the instruction is
    ArmInstruction getInstructionFormat(u32);

    // determine which type of thumb operation an instruction is
    ThumbInstruction getInstructionFormat(u16);

    // test if path exists
    bool pathExists(const string &);

    // given a 16 bit GBA color, make it a 32 bit SDL color
    inline u32 u16ToU32Color(u16);

    // signum function
    // https://en.wikipedia.org/wiki/Sign_function
    inline s8 signum(double);

    // util inline functions
    #include "util.inl"
}
