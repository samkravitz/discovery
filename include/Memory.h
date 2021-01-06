/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Memory.h
 * DATE: July 13, 2020
 * DESCRIPTION: GBA memory class defintion
 */
#pragma once

#include <stdlib.h>
#include <vector>

#include "LcdStat.h"
#include "common.h"
#include "Timer.h"
#include "mmio.h"
#include "log.h"

// start and end addresses of internal memory regions
constexpr u32 MEM_BIOS_END          = 0x3FFF;
constexpr u32 MEM_EWRAM_START       = 0x2000000;
constexpr u32 MEM_EWRAM_END         = 0x203FFFF;
constexpr u32 MEM_IWRAM_START       = 0x3000000;
constexpr u32 MEM_IWRAM_END         = 0x3007FFF;
constexpr u32 MEM_IO_REG_START      = 0x4000000;
constexpr u32 MEM_IO_REG_END        = 0x40003FE;
constexpr u32 MEM_PALETTE_RAM_START = 0x5000000;
constexpr u32 MEM_PALETTE_RAM_END   = 0x50003FF;
constexpr u32 MEM_VRAM_START        = 0x6000000;
constexpr u32 MEM_VRAM_END          = 0x6017FFF;
constexpr u32 MEM_OAM_START         = 0x7000000;
constexpr u32 MEM_OAM_END           = 0x70003FF;

constexpr u32 MEM_GAMEPAK_ROM_START = 0x8000000;
constexpr u32 MEM_GAMEPAK_ROM_END   = 0xE00FFFF;

// sizes of internal regions
constexpr u32 MEM_BIOS_SIZE        = 0x4000;
constexpr u32 MEM_EWRAM_SIZE       = 0x40000;
constexpr u32 MEM_IWRAM_SIZE       = 0x8000;
constexpr u32 MEM_IO_REG_SIZE      = 0x400;
constexpr u32 MEM_PALETTE_RAM_SIZE = 0x400;
constexpr u32 MEM_VRAM_SIZE        = 0x20000;
constexpr u32 MEM_OAM_SIZE         = 0x400;
constexpr u32 MEM_SIZE             = 0x8000000;

class Memory
{
    public:
        Memory();
        ~Memory();

        u8 memory[MEM_SIZE];

        LcdStat *stat;

        // cart buffers & sizes
        u8  cart_rom[0x2000000];
        u8 *cart_ram;
        size_t rom_size;
        size_t ram_size;
        
        struct DMA
        {
            u16 num_transfers;
            u8  dest_adjust : 2; // destination adjustment
            u8  src_adjust  : 2;  // source adjustment
            u8  repeat      : 1;
            u8  chunk_size  : 1;
            u8  mode        : 2;
            u8  irq         : 1;
            u8  enable      : 1;

            u32 src_address;
            u32 dest_address;
        } dma[4];

        Timer *timers[4];

        u8 n_cycles;
        u8 s_cycles;

        void Reset();
        bool LoadRom(const std::string &);
        bool LoadBios(const std::string &);

        // read / write from memory
        u32  Read32(u32);
        u16  Read16(u32);
        u8   Read8(u32);
        void Write8(u32, u8);
        void Write16(u32, u16);
        void Write32(u32, u32);

        // unprotected read/write from memory
        // dangerous, but fast
        u32  Read32Unsafe(u32);
        u16  Read16Unsafe(u32);
        u8   Read8Unsafe(u32);
        void Write8Unsafe(u32, u8);
        void Write16Unsafe(u32, u16);
        void Write32Unsafe(u32, u32);

        // DMA transfer routine
        void _Dma(int);

    private:
        void Dma0();
        void Dma1();
        void Dma2();
        void Dma3();
};