/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Timer.cpp
 * DATE: April 13th, 2021
 * DESCRIPTION: global IRQ manager
 */
#pragma once

#include "common.h"

class IRQ
{
public:
    IRQ();
    ~IRQ() = default;

    u16 reg_ie;  // IE
    u16 reg_if;  // IF
    u16 reg_ime; // IME

};