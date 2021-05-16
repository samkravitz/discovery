/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Timer.h
 * DATE: November 4th, 2020
 * DESCRIPTION: timer definition
 */

#pragma once

#include "common.h"

class Timer
{
public:
    Timer();

    u16 read(int);

    void tick();
    void write(int, u16);
    void writeCnt(int, u16);

private:
    long ticks;

    struct Channel
    {
        union
        {
            struct 
            {
                u8 freq    : 2;
                u8 cascade : 1;
                u8 unused1 : 3;
                u8 irq     : 1;
                u8 enable  : 1;
                u8 unused2 : 8;
            };

            u16 cnt; 
            
        };

        bool registered;
        u16 initial;
        u16 data;
        int prescalar;

    } channel[4];

    void cascade(int);
};