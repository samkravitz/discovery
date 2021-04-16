/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Timer.cpp
 * DATE: April 13th, 2021
 * DESCRIPTION: global IRQ manager
 */
#pragma once

#include "common.h"

// IRQ bits
constexpr u16 IRQ_VBLANK  = 1 << 0;
constexpr u16 IRQ_HBLANK  = 1 << 1;
constexpr u16 IRQ_VCOUNT  = 1 << 2;
constexpr u16 IRQ_TIMER0  = 1 << 3;
constexpr u16 IRQ_TIMER1  = 1 << 4;
constexpr u16 IRQ_TIMER2  = 1 << 5;
constexpr u16 IRQ_TIMER3  = 1 << 6;
constexpr u16 IRQ_COM     = 1 << 7;
constexpr u16 IRQ_DMA0    = 1 << 8;
constexpr u16 IRQ_DMA1    = 1 << 9;
constexpr u16 IRQ_DMA2    = 1 << 10;
constexpr u16 IRQ_DMA3    = 1 << 11;
constexpr u16 IRQ_KEYPAD  = 1 << 12;
constexpr u16 IRQ_GAMEPAK = 1 << 13;

class IRQ
{
public:
    IRQ();
    ~IRQ() = default;

    void raise(InterruptOccasion);
    void clear(u16);
    void enable();
    void disable();
    bool isEnabled();

    // getters / setters
    u16  getIE()  { return m_ie; }
    u16  getIF()  { return m_if; }
    u16  getIME() { return m_ime; }

    void setIE(u16 val)  { m_ie  = val;}
    void setIF(u16 val)  { m_if  = val;}
    void setIME(u16 val) { m_ime = val;}
    
private:
    // preface these with m_ so we don't use keyword if
    u16 m_ie;  // enabled interrupts
    u16 m_if;  // requested interrupts
    u16 m_ime; // master enable

};