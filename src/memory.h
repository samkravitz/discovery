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

#include "lcd_stat.h"
#include "common/common.h"
#include "common/memory.h"

class Memory
{
    public:
        Memory();
        ~Memory();

        u8 memory[MEM_SIZE];

        lcd_stat *stat;
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