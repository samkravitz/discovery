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
    // zero timers
    data = 0;
    start_data;
    freq = 0;
    cascade = 0;
    irq = 0;
    enable = 0;
    actual_freq = 0;
}

timer::~timer() { }