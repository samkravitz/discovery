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

class Memory {
    public:
        Memory();
        ~Memory();

        struct memory_struct {
            u8 bios[MEM_BIOS_SIZE];
            u8 board_wram[MEM_BOARD_WRAM_SIZE];
            u8 chip_wram[MEM_CHIP_WRAM_SIZE];
            u8 io_reg [MEM_IO_REG_SIZE];
            u8 palette_ram[MEM_PALETTE_RAM_SIZE];
            u8 vram[MEM_VRAM_SIZE];
            u8 oam[MEM_OAM_SIZE];
        } memory;

        lcd_stat *stat;

        u8 * game_rom;
        
        void load_rom(char *);
        void load_bios();

        // read / write from memory
        u32 read_u32(u32);
        u16 read_u16(u32);
        u8 read_u8(u32);
        void write_u8(u32, u8);
        void write_u16(u32, u16);
        void write_u32(u32, u32);

    private:
        u8 *get_internal_region(u32);
        std::size_t size;
};

#endif // MEMORY_H