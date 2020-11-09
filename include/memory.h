/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: memory.h
 * DATE: July 13, 2020
 * DESCRIPTION: GBA memory class defintion
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <vector>

#include "lcd_stat.h"
#include "common.h"
#include "timer.h"

// start and end addresses of internal memory regions
#define MEM_BIOS_END          0x3FFF
#define MEM_EWRAM_START       0x2000000
#define MEM_EWRAM_END         0x203FFFF
#define MEM_IWRAM_START       0x3000000
#define MEM_IWRAM_END         0x3007FFF
#define MEM_IO_REG_START      0x4000000
#define MEM_IO_REG_END        0x40003FE
#define MEM_PALETTE_RAM_START 0x5000000
#define MEM_PALETTE_RAM_END   0x50003FF
#define MEM_VRAM_START        0x6000000
#define MEM_VRAM_END          0x6017FFF
#define MEM_OAM_START         0x7000000
#define MEM_OAM_END           0x70003FF

#define MEM_GAMEPAK_ROM_START 0x8000000
#define MEM_GAMEPAK_ROM_END   0xE00FFFF 

// sizes of internal regions
#define MEM_BIOS_SIZE        0x4000
#define MEM_EWRAM_SIZE       0x40000
#define MEM_IWRAM_SIZE       0x8000
#define MEM_IO_REG_SIZE      0x400
#define MEM_PALETTE_RAM_SIZE 0x400
#define MEM_VRAM_SIZE        0x20000
#define MEM_OAM_SIZE         0x400
#define MEM_SIZE             0x8000000

class Memory
{
    public:
        Memory();
        ~Memory();

        u8 memory[MEM_SIZE];

        lcd_stat *stat;

        size_t rom_size;
        u8 *game_rom;
        
        struct dma
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

        timer *timers[4];

        u8 n_cycles;
        u8 s_cycles;

        void reset();
        void load_rom(char *);
        void load_bios();

        // read / write from memory
        u32 read_u32(u32);
        u16 read_u16(u32);
        u8 read_u8(u32);
        void write_u8(u32, u8);
        void write_u16(u32, u16);
        void write_u32(u32, u32);

        // unprotected read/write from memory
        // dangerous, but fast
        u32 read_u32_unprotected(u32);
        u16 read_u16_unprotected(u32);
        u8 read_u8_unprotected(u32);
        void write_u8_unprotected(u32, u8);
        void write_u16_unprotected(u32, u16);
        void write_u32_unprotected(u32, u32);

        // DMA transfer routine
        void _dma(int);

    private:
        void dma0();
        void dma1();
        void dma2();
        void dma3();
};

#endif // MEMORY_H