/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: util.h
 * DATE: June 19, 2020
 * DESCRIPTION: utility functions that don't belong in any class
 */
#ifndef UTIL_H
#define UTIL_H

#include "common.h"
#include "cpu.h"

namespace util
{
    // determine which type of operation the instruction is
    instruction_set_format_t get_instruction_format(u32 instruction);

    // determine which type of thumb operation an instruction is
    thumb_instruction_format_t get_instruction_format(u16 instruction);

    // get subset of instruction for purposes like destination register, opcode, shifts
    u32 get_instruction_subset(u32, int, int);

    // get subset of thumb instruction
    u16 get_instruction_subset(u16, int, int);
}

#endif