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

struct Timer
{
    public:
        Timer();
        ~Timer();

        long ticks;

        struct channel
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

            u16 initial;
            u16 data;
            int prescalar;
        } channel[4];
        
        void Tick();
        u16  Read(int);
        void Write(int, u16);
        void WriteCnt(int, u16);

    private:
        void Cascade(int);
};