/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: swi.cpp
 * DATE: October 8, 2020
 * DESCRIPTION: software interrupt handlers
 */

#include "arm_7tdmi.h"

/*
 *
 * 
 * 
 */
void arm_7tdmi::swi_softreset()
{
    
}

/*
 * Resets the I/O registers and RAM specified in ResetFlags. However, it does not clear the CPU internal RAM area from 3007E00h-3007FFFh.
 * r0  ResetFlags
 *      Bit   Expl.
 *      0     Clear 256K on-board WRAM  ;-don't use when returning to WRAM
 *      1     Clear 32K on-chip WRAM    ;-excluding last 200h bytes
 *      2     Clear Palette
 *      3     Clear VRAM
 *      4     Clear OAM              ;-zerofilled! does NOT disable OBJs!
 *      5     Reset SIO registers    ;-switches to general purpose mode!
 *      6     Reset Sound registers
 *      7     Reset all other registers (except SIO, Sound)
 * 
 * The function always switches the screen into forced blank by setting DISPCNT=0080h
 * (regardless of incoming R0, screen becomes white).
 */
void arm_7tdmi::swi_register_ram_reset()
{
    u8 flags = get_register(0) & 0xFF;

    // bit 0
    if (flags & (1 << 0))
    {
        for (int i = 0; i < MEM_EWRAM_SIZE; ++i)
            mem->write_u8_unprotected(i + MEM_EWRAM_START, 0);
    }

    // bit 1
    if (flags & (1 << 1))
    {
        for (int i = 0; i < MEM_IWRAM_SIZE - 0x200; ++i)
            mem->write_u8_unprotected(i + MEM_IWRAM_START, 0);
    }

    // bit 2
    if (flags & (1 << 2))
    {
        for (int i = 0; i < MEM_PALETTE_RAM_SIZE; ++i)
            mem->write_u8_unprotected(i + MEM_PALETTE_RAM_START, 0);
    }

    // bit 3
    if (flags & (1 << 3))
    {
        for (int i = 0; i < MEM_VRAM_SIZE; ++i)
            mem->write_u8_unprotected(i + MEM_VRAM_START, 0);
    }

    // bit 4
    if (flags & (1 << 4))
    {
        for (int i = 0; i < MEM_OAM_SIZE; ++i)
            mem->write_u8_unprotected(i + MEM_OAM_START, 0);
    }

    // bit 5
    if (flags & (1 << 5))
    {
        
    }

    // bit 6
    if (flags & (1 << 6))
    {
        
    }

    // bit 7
    if (flags & (1 << 7))
    {
        
    }

    // force white screen
    mem->write_u32_unprotected(REG_DISPCNT, 0x0080);
}

/*
 * Signed division, r0 / r1
 * r0 signed 32 bit number
 * r1 signed 32 bit denom
 * 
 * return:
 * r0 - number DIV denom, signed
 * r1 - number MOD denom, signed
 * r3 abs(number DIV) denom, unsigned
 * 
 */
void arm_7tdmi::swi_division()
{
    s32 num = (s32) get_register(0);
    s32 denom = (s32) get_register(1);

    // divide by 0
    if (denom == 0)
    {
        std::cerr << "SWI DIV: dividing by 0!\n";
        return;
    }

    set_register(0, (u32) (num / denom));
    set_register(1, (u32) (num % denom));
    set_register(3, abs(num % denom));
}