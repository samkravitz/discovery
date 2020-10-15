/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: swi.cpp
 * DATE: October 8, 2020
 * DESCRIPTION: software interrupt handlers
 */

#include "arm_7tdmi.h"

/*
 * Signed division, r0 / r1
 * r0 signed 32 bit number
 * r1 signed 32 bit denom
 * 
 * return:
 * r0 - number DIV denom, signed
 * r1 - number MOD denom, signed
 * r3 abs(number DIV) denom, unsigned
 * 
 */
void arm_7tdmi::swi_division()
{
    s32 num = (s32) get_register(0);
    s32 denom = (s32) get_register(1);

    // divide by 0
    if (denom == 0)
    {
        std::cerr << "SWI DIV: dividing by 0!\n";
        return;
    }

    set_register(0, (u32) (num / denom));
    set_register(1, (u32) (num % denom));
    set_register(3, abs(num % denom));
}