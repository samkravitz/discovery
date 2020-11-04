/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: timer.cpp
 * DATE: November 4th, 2020
 * DESCRIPTION: timer definition
 */

#include "timer.h"

timer::timer()
{
    num = 0xFF;
    freq = 0;
    cascade = 0;
    irq = 0;
    enable = 0;
}

timer::~timer() { }