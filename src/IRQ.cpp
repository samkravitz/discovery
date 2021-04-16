/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Timer.cpp
 * DATE: April 13th, 2021
 * DESCRIPTION: global IRQ manager
 */
#include "IRQ.h"
#include <cassert>

IRQ::IRQ()
{
    m_ie  = 0;
    m_if  = 0;
    m_ime = 0;
}

void IRQ::raise(InterruptOccasion occaision)
{
    switch (occaision)
    {
        case InterruptOccasion::VBLANK:  m_if |= IRQ_VBLANK;  break;
        case InterruptOccasion::HBLANK:  m_if |= IRQ_HBLANK;  break;
        case InterruptOccasion::VCOUNT:  m_if |= IRQ_VCOUNT;  break;
        case InterruptOccasion::TIMER0:  m_if |= IRQ_TIMER0;  break;
        case InterruptOccasion::TIMER1:  m_if |= IRQ_TIMER1;  break;
        case InterruptOccasion::TIMER2:  m_if |= IRQ_TIMER2;  break;
        case InterruptOccasion::TIMER3:  m_if |= IRQ_TIMER3;  break;
        case InterruptOccasion::COM:     m_if |= IRQ_COM;     break;
        case InterruptOccasion::DMA0:    m_if |= IRQ_DMA0;    break;
        case InterruptOccasion::DMA1:    m_if |= IRQ_DMA1;    break;
        case InterruptOccasion::DMA2:    m_if |= IRQ_DMA2;    break;
        case InterruptOccasion::DMA3:    m_if |= IRQ_DMA3;    break;
        case InterruptOccasion::KEYPAD:  m_if |= IRQ_KEYPAD;  break;
        case InterruptOccasion::GAMEPAK: m_if |= IRQ_GAMEPAK; break;
        default:
            assert(!"Error: invalid occasion in IRQ::raise");
    }
}

void IRQ::clear(u16 val) { m_if &= ~val; }
void IRQ::enable()       { m_ime = 1; }
void IRQ::disable()      { m_ime = 0; }
bool IRQ::isEnabled()    { return m_ime != 0; }