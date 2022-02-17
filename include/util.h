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
#include <ctime>
#include <cmath>

#include "common.h"

using std::string;

namespace util
{
    // determine which type of operation the instruction is
    ArmInstruction getInstructionFormat(u32);

    // determine which type of thumb operation an instruction is
    ThumbInstruction getInstructionFormat(u16);

    // test if path exists
    bool pathExists(std::string const &);

    // util inline functions
    #include "util.inl"
}
