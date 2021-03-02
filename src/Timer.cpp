/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Timer.cpp
 * DATE: March 1st, 2021
 * DESCRIPTION: timer definition
 */

#include "Timer.h"
#include "common.h"

Timer::Timer()
{
    ticks = 0;
    
    // zero channels
    for (int i = 0; i < 4; ++i)
    {
        channel[i].cnt         = 0;
        channel[i].data        = 0;
        channel[i].initial     = 0;
        channel[i].prescalar = 1;
    }
}

Timer::~Timer() { }

u16 Timer::Read(int ch) { return channel[ch].data; }

void Timer::Write(int ch, u16 value)
{
    channel[ch].initial = value;
    channel[ch].data    = value;
}

void Timer::WriteCnt(int ch, u16 value)
{
    channel[ch].cnt = value;

    // set actual freq
    switch (channel[ch].freq)
    {
        case 0: channel[ch].prescalar =    1; break;
        case 1: channel[ch].prescalar =   64; break;
        case 2: channel[ch].prescalar =  256; break;
        case 3: channel[ch].prescalar = 1024; break;
    }
}

void Timer::Tick()
{
    ++ticks;

    for (int j = 0; j < 4; ++j)
    {
        // ignore if timer is disabled
        if (!channel[j].enable)
            continue;

        // ignore if cascade bit is set (timer will be incremented by previous timer)
        if (channel[j].cascade)
            continue;

        // increment counter by 1
        if (ticks % channel[j].prescalar == 0)
        {
            channel[j].data += 1; // increment timer

            // timer overflowed
            if (channel[j].data == 0x0000)
            {
                // reset timer
                channel[j].data = channel[j].initial;

                //std::cout << "Timer " << j << " overflow\n";

                // overflow irq
                if (channel[j].irq)
                    LOG("Timer {} overflow IRQ request\n");

                // cascade
                Cascade(j);
            }
        }
    }
}

void Timer::Cascade(int ch)
{
    // timer 3 can't cascade any other timer
    if (ch == 3)
        return;

    if (channel[ch + 1].enable && channel[ch + 1].cascade)
    {
        channel[ch + 1].data += 1;

        // cascade caused overflow (deal with this later)
        if (channel[ch + 1].data == 0x0000)
        {
            LOG("Timer {} cascade overflow\n", ch + 1);
            Cascade(ch + 1);
        }
            
    }
}

// struct Timer
// {
//     public:
//         Timer();
//         ~Timer();

//         long ticks;

//         struct channel
//         {
//             union
//             {
//                 struct 
//                 {
//                     u8 freq    : 2;
//                     u8 cascade : 1;
//                     u8 unused1 : 3;
//                     u8 irq     : 1;
//                     u8 enable  : 1;
//                     u8 unused2 : 8;
//                 };

//                 u16 cnt; 
                
//             };

//             u16 initial;
//             u16 data;
//         } channel[4];

//         u16  Read(int);
//         void Write(int, u16);
//         void WriteCnt(int, u16);
// };