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

namespace Util
{
    // determine which type of operation the instruction is
    ArmInstruction GetInstructionFormat(u32 instruction);

    // determine which type of thumb operation an instruction is
    ThumbInstruction GetInstructionFormat(u16 instruction);

    bool PathExists(std::string);

    // util inline functions
    #include "Util.inl"
}

