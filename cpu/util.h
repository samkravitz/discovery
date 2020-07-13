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

#include <stdint.h>
#include "common.h"
#include "arm_7tdmi.h"

namespace util {
    // determine if the condition field of an instruction is true, given the state of the CPSR
    bool condition_met(arm_instruction, arm_7tdmi &);

    // determine which type of operation the instruction is
    instruction_set_format_t get_instruction_format(arm_instruction instruction);

    // get subset of instruction for purposes like destination register, opcode, shifts
    uint32_t get_instruction_subset(arm_instruction, int, int);
}

#endif