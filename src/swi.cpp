/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: swi.cpp
 * DATE: October 8, 2020
 * DESCRIPTION: software interrupt handlers
 */

#include <cmath>
#include "arm_7tdmi.h"

/*
 *
 * 
 * 
 */
void arm_7tdmi::swi_softReset()
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
void arm_7tdmi::swi_registerRamReset()
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
 * r0 - signed 32 bit number
 * r1 - signed 32 bit denom
 * 
 * return:
 * r0 - number DIV denom, signed
 * r1 - number MOD denom, signed
 * r3 - abs(number DIV) denom, unsigned
 */
void arm_7tdmi::swi_division()
{
    s32 num   = (s32) get_register(0);
    s32 denom = (s32) get_register(1);

    // divide by 0
    if (denom == 0)
    {
        std::cerr << "SWI DIV: dividing by 0!\n";
        return;
    }

    set_register(0, (u32) (num / denom));
    set_register(1, (u32) (num % denom));
    set_register(3, abs(num / denom));
}

/*
 * Calculate square root
 * r0 - u32 operand
 * 
 * return:
 * r0 - u16 result
 */
void arm_7tdmi::swi_sqrt()
{
    u32 num    = get_register(0);
    u16 result = (u16) sqrt(num);

    set_register(0, result);
}

/*
 * Calculate the two param arctan
 * r0 - x 16 bit (1bit sign, 1bit integral part, 14bit decimal part)
 * r1 - y 16 bit (1bit sign, 1bit integral part, 14bit decimal part)
 * 
 * return:
 * r0 - 0x0000 - 0xFFFF for 0 <= theta <= 2π
 */
void arm_7tdmi::swi_arctan2()
{
    s16 x = get_register(0);
    s16 y = get_register(1);

    // TODO - handle case for negative x ?
    float result = atan2f(y, x);

    // arctan has range [0, 2π) but we want
    // result in range [0x0, 0xFFFF]
    result *= (0xFFFF / (2 * M_PI));

    set_register(0, (u16) result);
}

void arm_7tdmi::swi_cpuSet()
{
    u32 src_ptr  = get_register(0);
    u32 dest_ptr = get_register(1);
    u32 mode     = get_register(2);

    std::cout << "SWI CpuSet\n";
    std::cout << "Src Ptr: " << std::hex<< src_ptr << "\n";
    std::cout << "Dest Ptr: " << std::hex<< dest_ptr << "\n";
    std::cout << "Mode: " << std::hex<< mode << "\n";

    u32 h = 0x1FFFFF;
}

/*
 * ObjAffineSet
 */
void arm_7tdmi::swi_objAffineSet()
{
    u32 src_ptr          = get_register(0);
    u32 dest_ptr         = get_register(1);
    u32 num_calculations = get_register(2);
    u32 offset           = get_register(3);

    float sx, sy;         // scale x, y
	float alpha;          // angle of rotation
	float pa, pb, pc, pd; // calculated P matrix

    // // TODO - these more complex cases that I don't feel like doing now
    if (num_calculations != 1)
        std::cout << "SWI ObjAffineSet > 1 calculations\n";
    
    if (offset != 2) // for OAM, not bg
        std::cout << "SWI ObjAffineSet for OAM\n";

    // integer portion of 8.8f sx, sy
    sx = (float) (mem->read_u16(src_ptr    ) >> 8);
    sy = (float) (mem->read_u16(src_ptr + 2) >> 8);

    // convert alpha from range [0x0 - 0xFFFF] to [0, 2π]
    alpha = (mem->read_u16(src_ptr + 4)) / 32768.0 * M_PI;
    pa = pd = cosf(alpha);
    pb = pc = sinf(alpha);

    pa *=  sx; // sx *  cos(α)
    pb *= -sx; // sx * -sin(α)
    pc *=  sy; // sy *  sin(α)
    pd *=  sy; // sy *  cos(α)

    // convert back to range [0x0 - 0xFFFF]
    mem->write_u16(dest_ptr    , pa * 256);
    mem->write_u16(dest_ptr + 2, pb * 256);
    mem->write_u16(dest_ptr + 4, pc * 256);
    mem->write_u16(dest_ptr + 6, pd * 256);
}