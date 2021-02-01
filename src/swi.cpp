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
#include "Arm7Tdmi.h"

/*
 *
 * 
 * 
 */
void Arm7Tdmi::SwiSoftReset()
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
void Arm7Tdmi::SwiRegisterRamReset()
{
    u8 flags = GetRegister(r0) & 0xFF;

    // bit 0
    if (flags & (1 << 0))
    {
        for (int i = 0; i < MEM_EWRAM_SIZE; ++i)
            mem->Write8Unsafe(i + MEM_EWRAM_START, 0);
    }

    // bit 1
    if (flags & (1 << 1))
    {
        for (int i = 0; i < MEM_IWRAM_SIZE - 0x200; ++i)
            mem->Write8Unsafe(i + MEM_IWRAM_START, 0);
    }

    // bit 2
    if (flags & (1 << 2))
    {
        for (int i = 0; i < MEM_PALETTE_RAM_SIZE; ++i)
            mem->Write8Unsafe(i + MEM_PALETTE_RAM_START, 0);
    }

    // bit 3
    if (flags & (1 << 3))
    {
        for (int i = 0; i < MEM_VRAM_SIZE; ++i)
            mem->Write8Unsafe(i + MEM_VRAM_START, 0);
    }

    // bit 4
    if (flags & (1 << 4))
    {
        for (int i = 0; i < MEM_OAM_SIZE; ++i)
            mem->Write8Unsafe(i + MEM_OAM_START, 0);
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
    mem->Write32Unsafe(REG_DISPCNT, 0x0080);
}

/*
 * VBlank interrupt wait
 * 
 * Halts execution until a VBlank interrupt arises
 */
void Arm7Tdmi::SwiVBlankIntrWait()
{
    // force interrupts to be enabled
    // mem->Write32Unsafe(REG_IME, 0x1);
    // registers.cpsr.flags.i = 0;

    // // write 1 to r0, r1
    // SetRegister(r0, 0x1);
    // SetRegister(r1, 0x1);

    // cycles = cycles + 197120 - cycles;
    // std::cout << "a" << ((int) mem->read_u32_unprotected(REG_IF)) << "\n";
    // std::cout << "b" << ((int) mem->read_u32_unprotected(REG_IE)) << "\n";
    // std::cout << ((int) mem->read_u32_unprotected(REG_IME)) << "\n";
    //exit(0);
    // registers.r15 -= GetState() == State::ARM ? 4 : 2;
    // pipeline[1] = pipeline[0];
    // pipeline[2] = pipeline[0];
    // swi_vblank_intr = true;
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
void Arm7Tdmi::SwiDivision()
{
    s32 num   = (s32) GetRegister(r0);
    s32 denom = (s32) GetRegister(r1);

    // divide by 0
    if (denom == 0)
    {
        LOG(LogLevel::Error, "SWI DIV: dividing by 0!\n");
        return;
    }

    SetRegister(r0, (u32) (num / denom));
    SetRegister(r1, (u32) (num % denom));
    SetRegister(r3, abs(num / denom));
}

/*
 * Calculate square root
 * r0 - u32 operand
 * 
 * return:
 * r0 - u16 result
 */
void Arm7Tdmi::SwiSqrt()
{
    u32 num    = GetRegister(r0);
    u16 result = (u16) sqrt(num);

    SetRegister(r0, result);
}

/*
 * Calculate the two param arctan
 * r0 - x 16 bit (1bit sign, 1bit integral part, 14bit decimal part)
 * r1 - y 16 bit (1bit sign, 1bit integral part, 14bit decimal part)
 * 
 * return:
 * r0 - 0x0000 - 0xFFFF for 0 <= theta <= 2π
 */
void Arm7Tdmi::SwiArctan2()
{
    s16 x = GetRegister(r0);
    s16 y = GetRegister(r1);

    // TODO - handle case for negative x ?
    float result = atan2f(y, x);

    // arctan has range [0, 2π) but we want
    // result in range [0x0, 0xFFFF]
    result *= (0xFFFF / (2 * M_PI));

    SetRegister(r0, (u16) result);
}

void Arm7Tdmi::SwiCpuSet()
{
    u32 src_ptr  = GetRegister(r0);
    u32 dest_ptr = GetRegister(r1);
    u32 mode     = GetRegister(r2);

    u32 wordcount = mode & 0x1FFFFF; // bits 0-20
    u8 fill       = mode >> 24 & 1; // bit 24 (1 == fill, 0 == copy)
    u8 datasize   = (mode >> 26 & 1) == 0 ? 2 : 4;

    u32 val32;
    u16 val16;

    // memfill
    if (fill == 1)
    {
        // get word / halfword
        if (datasize == 2)
        {
            // align src and dest addr
            src_ptr  &= ~0x1;
            dest_ptr &= ~0x1;

            val16 = mem->Read16(src_ptr);
            for (int i = 0; i < wordcount; ++i)
            {
                mem->Write16(dest_ptr, val16);
                dest_ptr += datasize;
            }
        }

        else
        {
            // align src and dest addr
            src_ptr  &= ~0x3;
            dest_ptr &= ~0x3;

            val32 = mem->Read32(src_ptr);
            for (int i = 0; i < wordcount; ++i)
            {
                mem->Write32(dest_ptr, val32);
                dest_ptr += datasize;
            } 
        }
    }

    // memcpy
    else
    {
        for (int i = 0; i < datasize; ++i)
        {
            // halfword transfer
            if (datasize == 2)
            {
                // align src and dest addr
                src_ptr  &= ~0x1;
                dest_ptr &= ~0x1;

                for (int i = 0; i < wordcount; ++i)
                {
                    val16 = mem->Read16(src_ptr);
                    mem->Write16(dest_ptr, val16);
                    dest_ptr += datasize;
                    src_ptr += datasize;
                }
            }
                
            else
            {
                // align src and dest addr
                src_ptr  &= ~0x3;
                dest_ptr &= ~0x3;

                for (int i = 0; i < wordcount; ++i)
                {
                    val32 = mem->Read32(src_ptr);
                    mem->Write32(dest_ptr, val32);
                    dest_ptr += datasize;
                    src_ptr += datasize;
                }
            }
        }
    }

    // std::cout << "SWI CpuSet\n";
    // std::cout << "Src Ptr: "   << std::hex << src_ptr << "\n";
    // std::cout << "Dest Ptr: "  << std::hex << dest_ptr << "\n";
    // std::cout << "Mode: "      << std::hex << mode << "\n";
    // std::cout << "Wordcount: " << std::hex << wordcount << "\n";
    // std::cout << "fill: "      << std::hex << (int) fill << "\n";
    // std::cout << "datasize: "  << std::hex << (int) datasize << "\n";
}

/*
 * ObjAffineSet
 */
void Arm7Tdmi::SwiObjAffineSet()
{
    //return;
    u32 src_ptr          = GetRegister(r0);
    u32 dest_ptr         = GetRegister(r1);
    u32 num_calculations = GetRegister(r2);
    u32 offset           = GetRegister(r3);

    float sx, sy;         // scale x, y
	float alpha;          // angle of rotation
	float pa, pb, pc, pd; // calculated P matrix

    // TODO - are src. dest ptrs updated correctly for multiple calculations
    if (num_calculations != 1)
        std::cout << "SWI ObjAffineSet > 1 calculations\n";

    for (int i = 0; i < num_calculations; ++i)
    {
        // integer portion of 8.8f sx, sy
        sx = (float) (mem->Read16(src_ptr    ) >> 8);
        sy = (float) (mem->Read16(src_ptr + 2) >> 8);

        // convert alpha from range [0x0 - 0xFFFF] to [0, 2π]
        alpha = (mem->Read16(src_ptr + 4)) / 32768.0 * M_PI;
        pa = pd = cosf(alpha);
        pb = pc = sinf(alpha);

        pa *=  sx; // sx *  cos(α)
        pb *= -sx; // sx * -sin(α)
        pc *=  sy; // sy *  sin(α)
        pd *=  sy; // sy *  cos(α)

        // convert back to range [0x0 - 0xFFFF]
        mem->Write16(dest_ptr, pa * 256); dest_ptr += offset;
        mem->Write16(dest_ptr, pb * 256); dest_ptr += offset;
        mem->Write16(dest_ptr, pc * 256); dest_ptr += offset;
        mem->Write16(dest_ptr, pd * 256); dest_ptr += offset;

        src_ptr  += offset;
    }
}

/*
 *  Bit Unpack
 * 
 *  r0  Source Address      (no alignment required)
 *  r1  Destination Address (must be 32bit-word aligned)
 *  r2  Pointer to UnPack information:
 *      16bit  Length of Source Data in bytes     (0-FFFFh)
 *      8bit   Width of Source Units in bits      (only 1,2,4,8 supported)
 *      8bit   Width of Destination Units in bits (only 1,2,4,8,16,32 supported)
 *      32bit  Data Offset (Bit 0-30), and Zero Data Flag (Bit 31)
 */
void Arm7Tdmi::SwiBitUnpack()
{
    u32 src_ptr     = GetRegister(r0);
    u32 dest_ptr    = GetRegister(r1) & ~0x3;
    u32 info_ptr    = GetRegister(r2);

    u32 info_lower  = mem->Read32(info_ptr);
    u32 data_offset = mem->Read32(info_ptr + 4);

    bool zero_flag = (data_offset >> 31) == 1;
    data_offset &= 0x7FFFFFFF; // clear MSB

    u16 len         = info_lower & 0xFFFF;
    u8 src_width    = (info_lower >> 16) & 0xFF; // in bits
    u8 dest_width   = (info_lower >> 24) & 0xFF; // in bits

    // only 1, 2, 4, 8 supported for src
    switch (src_width)
    {
        case 1:
        case 2:
        case 4:
        case 8:
            break;
        
        default:
            std::cout << "Invalid src width given for SWI::bitUnpack " << (int) src_width << "\n";
            return;
    }

    // only 1, 2, 4, 8, 16, 32 supported for dest
    switch (dest_width)
    {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
            break;
        
        default:
            std::cout << "Invalid dest width given for SWI::bitUnpack " << (int) dest_width << "\n";
            return;
    }

    // algorithm taken from
    // https://github.com/Cult-of-GBA/BIOS/blob/master/bios_calls/decompression/bitunpack.s
    u8 src_bit_count;
    u8 dest_bit_count = 0;
    u32 buffer = 0;
    u8 mask, data, masked_data;
    for (int i = 0; i < len; ++i)
    {
        
        data = mem->Read8(src_ptr++);
        src_bit_count = 0;
        mask = 0xFF >> (8 - src_width);

        while (src_bit_count < 8)
        {
            masked_data = (data & mask) >> src_bit_count;
            if (masked_data != 0 || zero_flag)
                masked_data += data_offset;
            
            buffer |= masked_data << dest_bit_count;
            dest_bit_count += dest_width;

            if (dest_bit_count >= 32)
            {
                mem->Write32(dest_ptr, buffer);
                dest_ptr += sizeof(u32);
                dest_bit_count = 0;
                buffer = 0;
            }

            mask <<= src_width;
            src_bit_count += src_width;
        }
    }

    // std::cout << "SWI 0x10 - Bit unpack\n";
    // std::cout << "len: " << std::hex << (int) len << "\n";
    // std::cout << "src_width: " << (int) src_width << "\n";
    // std::cout << "dest_width: " << (int) dest_width << "\n";
}

void Arm7Tdmi::SwiRLUnCompVRAM()
{
    u32 src_ptr, dest_ptr;
    src_ptr  = GetRegister(r0) & ~0x3; // word aligned
    dest_ptr = GetRegister(r1) & ~0x1; // halword aligned

    // bits 0-3 reserved, 4-7 compressed type (3), 8-31 size of compressed data
    u32 data_header = mem->Read32(src_ptr);
    u32 decomp_len = data_header >> 8;

    src_ptr += 4;

    while (decomp_len > 0)
    {
        u8 flags = mem->Read8(src_ptr++);
        u8 expand_len = flags & 0x7F;

        if ((flags >> 7) == 0) // uncompressed
        {
            expand_len++;
            decomp_len -= expand_len;

            while (decomp_len > 0)
            {
                u16 data = mem->Read16(src_ptr);
                mem->Write16(dest_ptr, data);

                src_ptr  += 2;
                dest_ptr += 2;
                decomp_len--;
            }
        }

        else
        {
            expand_len += 3;
            decomp_len -= expand_len;

            u16 data = mem->Read16(src_ptr);
            while (decomp_len > 0)
            {
                mem->Write16(dest_ptr, data);

                dest_ptr += 2;
                decomp_len--;
            }
        }
        
    }
    // bits 0-6 expanded data length (uncompressed n - 1, compressed n - 3), bit 7 flag (0 = uncompresed, 1 = compressed)


    
    // std::cout << "SWI 0x10 - Bit unpack\n";
    // std::cout << "len: " << std::hex << (int) len << "\n";
    // std::cout << "src_width: " << (int) src_width << "\n";
    // std::cout << "dest_width: " << (int) dest_width << "\n";
}
