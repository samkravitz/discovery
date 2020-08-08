/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: memory.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of memory related functions
 */
#include "memory.h"

#include <fstream>
#include <iostream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

Memory::Memory() {
    for (int i = 0; i < MEM_BIOS_SIZE; ++i) memory.bios[i] = 0;
    for (int i = 0; i < MEM_BOARD_WRAM_SIZE; ++i) memory.board_wram[i] = 0;
    for (int i = 0; i < MEM_VRAM_SIZE; ++i) memory.vram[i] = 0;
    // io_reg, palette, and oam regions all have the same size
    for (int i = 0; i < MEM_IO_REG_SIZE; ++i) {
        memory.io_reg[i] = 0;
        memory.palette_ram[i] = 0;
        memory.oam[i] = 0;
    }

    game_rom = NULL;
}

Memory::~Memory() { }

u32 Memory::read_u32(u32 address) {
    u8 *normalized_address = get_internal_region(address);
    u32 value = 0;
    value |= *(normalized_address + 3);
    value <<= 8;
    value |= *(normalized_address + 2);
    value <<= 8;
    value |= *(normalized_address + 1);
    value <<= 8;
    value |= *normalized_address;
    return value;
}

u16 Memory::read_u16(u32 address) {
    u8 *normalized_address = get_internal_region(address);
    u16 value = 0;
    value |= *(normalized_address + 1);
    value <<= 8;
    value |= *normalized_address;
    return value;
}

u8 Memory::read_u8(u32 address) {
    return *get_internal_region(address);
}

void Memory::write_u32(u32 address, u32 value) {
    u8 *normalized_address = get_internal_region(address);
    *normalized_address = value & 0xFF;
    *(normalized_address + 1) = (value >> 8) & 0xFF;
    *(normalized_address + 2) = (value >> 16) & 0xFF;
    *(normalized_address + 3) = (value >> 24) & 0xFF;
}

void Memory::write_u16(u32 address, u16 value) {
    u8 *normalized_address = get_internal_region(address);
    *normalized_address = value & 0xFF;
    *(normalized_address + 1) = (value >> 8) & 0xFF;
}

void Memory::write_u8(u32 address, u8 value) {
    u8 *normalized_address = get_internal_region(address);
    *normalized_address = value;
}

void Memory::load_rom(char *name) {
    std::ifstream rom(name, std::ios::in | std::ios::binary);
    if (!rom) return;
    size_t size = fs::file_size(name);

    if (!rom.good()) {
        std::cerr << "Bad rom!" << "\n";
        return;
    }
    game_rom = new u8[size]();
    rom.read((char *) game_rom, size);
    rom.close();
}

/*
 * GBA memory can be addressed anywhere from 0x00000000-0xFFFFFFFF, however most of those addresses are unused.
 * given a 4 u8 address, this function will return the address of the
 * internal region the address points to, which saves a boatload of memory.
 */
u8 *Memory::get_internal_region(u32 address) {
    if (address <= MEM_BIOS_END) return &memory.bios[address];
    else if (address >= MEM_BOARD_WRAM_START && address <= MEM_BOARD_WRAM_END) return &memory.board_wram[address - MEM_BOARD_WRAM_START];
    else if (address >= MEM_CHIP_WRAM_START && address <= MEM_CHIP_WRAM_END) return &memory.chip_wram[address - MEM_CHIP_WRAM_START];
    else if (address >= MEM_IO_REG_START && address <= MEM_IO_REG_END) return &memory.io_reg[address - MEM_IO_REG_START];
    else if (address >= MEM_PALETTE_RAM_START && address <= MEM_PALLETTE_RAM_END) return &memory.palette_ram[address - MEM_PALETTE_RAM_START];
    else if (address >= MEM_VRAM_START && address <= MEM_VRAM_END) return &memory.vram[address - MEM_VRAM_START];
    else if (address >= MEM_OAM_START && address <= MEM_OAM_END) return &memory.oam[address - MEM_OAM_START];
    else if (address >= MEM_GAMEPAK_ROM_START && address <= MEM_GAMEPAK_ROM_END) return &game_rom[address - MEM_GAMEPAK_ROM_START];
    else {
        std::cerr << "Error: invalid internal address specified: " << address << "\n";
        exit(2);
    }
}
