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

Timer::Timer(Scheduler *scheduler) :
    scheduler(scheduler)
{   
    // zero channels
    for (int i = 0; i < 4; ++i)
    {
        auto &tmr = channel[i];
        tmr.cnt           = 0;
        tmr.data          = 0;
        tmr.reload        = 0;
        tmr.cycle_started = 0;
        tmr.prescalar     = 1;

        tmr.onOverflow = [i, this]() {
            return std::bind(&Timer::overflow, this, i);
        }();
    }
}

u16 Timer::read(int ch)
{
    auto &tmr = channel[ch];

    if (tmr.cascade)
        return tmr.data;

    u64 diff = scheduler->cycles - tmr.cycle_started;
    return tmr.reload + diff / tmr.prescalar;
}

void Timer::write(int ch, u16 value)
{
    channel[ch].reload = value;
}

void Timer::writeCnt(int ch, u16 value)
{
    auto &tmr = channel[ch];
    bool was_off = tmr.enable;

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
        if (was_off)
            tmr.data = tmr.reload;
        
        tmr.cycle_started = scheduler->cycles;

        /*
         * find how many cycles until timer overflows
         * overflows when:
         * data + cycles / prescalar = 0xFFFF
         *
         * therefore:
         * cycles = (0xFFFF - data) * prescalar;
         */
        int cycles_until_overflow = (0xFFFF - tmr.data) * tmr.prescalar;
        scheduler->add(cycles_until_overflow, tmr.onOverflow, ch);
    }

    // remove tick event from scheduler
    else
        scheduler->remove(ch);
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

void Timer::overflow(int ch)
{   
    auto &tmr = channel[ch];
    tmr.data = tmr.reload;

    // overflow irq
    if (tmr.irq)
    {
        LOG("Timer {} IRQ\n", ch);
        
        switch (ch)
        {
            case 0: irq->raise(InterruptOccasion::TIMER0); break;
            case 1: irq->raise(InterruptOccasion::TIMER1); break;
            case 2: irq->raise(InterruptOccasion::TIMER2); break;
            case 3: irq->raise(InterruptOccasion::TIMER3); break;
        }
    }

    tmr.cycle_started = scheduler->cycles;

    // add next overflow event to scheduler
    int cycles_until_overflow = (0xFFFF - tmr.data) * tmr.prescalar;
    scheduler->add(cycles_until_overflow, tmr.onOverflow, ch);

    // cascade
    cascade(ch);
}