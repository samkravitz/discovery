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
#include <stdint.h>
#include "common.h"
#include "../cpu/common.h"
#include "../common.h"

class Memory {
    public:
        Memory();
        ~Memory();

        struct memory_struct {
            u8 bios[0x4000];
            u8 board_wram[0x40000];
            u8 chip_wram[0x8000];
            u8 io_reg [0x400];
            u8 palette_ram[0x400];
            u8 vram[0x18000];
            u8 oam[0x400];
        } memory;

        u8 * game_rom;
        
        void load_rom(char *);

        // read / write from memory
        word read_u32(word);
        halfword read_u16(word);
        byte read_u8(word);
        void write_u8(word, byte);
        void write_u16(word, halfword);
        void write_u32(word, word);

    private:
        u8 *get_internal_region(u32);
        std::size_t size;
};

#endif // MEMORY_H