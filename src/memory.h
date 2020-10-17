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

        u8 memory[0x8000000];

        lcd_stat *stat;
        u8 *game_rom;
        
        u8 n_cycles;
        u8 s_cycles;

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

    private:
        u8 *get_internal_region(u32);
        std::size_t size;
};

#endif // MEMORY_H