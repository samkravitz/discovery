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
#include <iostream>

extern IRQ *irq;

Timer::Timer(Scheduler *scheduler) :
    scheduler(scheduler)
{   
    // zero channels
    for (int i = 0; i < 4; ++i)
    {
        channel[i].cnt        = 0;
        channel[i].data       = 0;
        channel[i].initial    = 0;
        channel[i].prescalar  = 1;
        channel[i].registered = false;
        channel[i].event_handler = [i, this]() {
            return std::bind(&Timer::tick, this, i);
        }();
    }
}

u16 Timer::read(int ch) { return channel[ch].data; }

void Timer::write(int ch, u16 value)
{
    channel[ch].initial = value;
    channel[ch].data    = value;
}

void Timer::writeCnt(int ch, u16 value)
{
    auto &tmr = channel[ch];

    tmr.cnt = value;

    // set actual freq
    switch (tmr.freq)
    {
        case 0: tmr.prescalar =    1; break;
        case 1: tmr.prescalar =   64; break;
        case 2: tmr.prescalar =  256; break;
        case 3: tmr.prescalar = 1024; break;
    }

    // Don't schedule callback when cascade is set
    if (tmr.cascade)
       return;

    if (tmr.enable)
    {   
       if (!tmr.registered) {
        // Set scheduler callback handler
        scheduler->add(tmr.prescalar, tmr.event_handler, ch);
        LOG("{} {}\n", ch, tmr.prescalar);
        
       }

       tmr.registered = true;
    }

    else if (tmr.registered)
    {
        // remove tick event from scheduler
        scheduler->remove(ch);
        tmr.registered = false;
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

void Timer::tick(int ch)
{   
    auto &tmr = channel[ch];

    tmr.data += 1; // increment timer

    // timer overflowed
    if (tmr.data == 0x0000)
    {
        // reset timer
        tmr.data = tmr.initial;

        // overflow irq
        if (tmr.irq)
        {
            //LOG("Timer {} overflow IRQ request\n", i);
            switch (ch)
            {
                case 0: irq->raise(InterruptOccasion::TIMER0); break;
                case 1: irq->raise(InterruptOccasion::TIMER1); break;
                case 2: irq->raise(InterruptOccasion::TIMER2); break;
                case 3: irq->raise(InterruptOccasion::TIMER3); break;
            }
        }

        // cascade
        cascade(ch);
    }
    
    scheduler->add(tmr.prescalar, tmr.event_handler, ch);
}