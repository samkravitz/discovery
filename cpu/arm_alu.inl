/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: arm_alu.inl
 * DATE: June 27, 2020
 * DESCRIPTION: execution of arm instructions
 */

#include <iostream>

#include "arm_7tdmi.h"

inline void arm_7tdmi::branch_exchange(arm_instruction instruction) {

}

inline void executeALUInstruction(arm_7tdmi &arm, arm_instruction instruction) {
    std::cout << "Got to the ALU!\n";
}