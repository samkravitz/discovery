/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: timer.h
 * DATE: November 4th, 2020
 * DESCRIPTION: timer definition
 */

#include "common/common.h"

struct timer
{
    public:
        timer();
        ~timer();

        u32 cycles;
    
    private:
        void cycle();

        u8 num; // timer number
        u8 freq;
        u8 cascade;
        u8 irq;
        u8 enable;
};