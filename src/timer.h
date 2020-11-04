/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: timer.h
 * DATE: November 4th, 2020
 * DESCRIPTION: timer definition
 */

#ifndef TIMER_H
#define TIMER_H

#include "common/common.h"

struct timer
{
    public:
        timer();
        ~timer();

        u16 data;
        u16 start_data;
        u8 freq;
        u8 cascade;
        u8 irq;
        u8 enable;
        u16 actual_freq;

};

#endif // TIMER_H