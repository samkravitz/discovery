/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Timer.cpp
 * DATE: March 1st, 2021
 * DESCRIPTION: timer definition
 */

#include "Timer.h"
#include "IRQ.h"

extern IRQ *irq;

Timer::Timer()
{
    ticks = 0;
    
    // zero channels
    for (int i = 0; i < 4; ++i)
    {
        channel[i].cnt       = 0;
        channel[i].data      = 0;
        channel[i].initial   = 0;
        channel[i].prescalar = 1;
    }
}

Timer::~Timer() { }

u16 Timer::read(int ch) { return channel[ch].data; }

void Timer::write(int ch, u16 value)
{
    channel[ch].initial = value;
    channel[ch].data    = value;
}

void Timer::writeCnt(int ch, u16 value)
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

void Timer::tick()
{
    ++ticks;

    for (int i = 0; i < 4; ++i)
    {
        // ignore if timer is disabled
        if (!channel[i].enable)
            continue;

        // ignore if cascade bit is set (timer will be incremented by previous timer)
        if (channel[i].cascade)
            continue;

        // increment counter by 1
        if (ticks % channel[i].prescalar == 0)
        {
            channel[i].data += 1; // increment timer

            // timer overflowed
            if (channel[i].data == 0x0000)
            {
                // reset timer
                channel[i].data = channel[i].initial;

                // overflow irq
                if (channel[i].irq)
                {
                    LOG("Timer {} overflow IRQ request\n", i);
                    switch (i)
                    {
                        case 0: irq->raise(InterruptOccasion::TIMER0); break;
                        case 1: irq->raise(InterruptOccasion::TIMER1); break;
                        case 2: irq->raise(InterruptOccasion::TIMER2); break;
                        case 3: irq->raise(InterruptOccasion::TIMER3); break;
                    }
                }

                // cascade
                cascade(i);
            }
        }
    }
}

void Timer::cascade(int ch)
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
            cascade(ch + 1);
        }
            
    }
}