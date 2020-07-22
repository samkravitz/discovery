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
    memory = new uint8_t[GBA_MEM_SIZE]();
}

Memory::~Memory() {
    delete memory;
}

word Memory::read_u32(word address) {
    word value = 0;
    value |= memory[address + 3];
    value <<= 8;
    value |= memory[address + 2];
    value <<= 8;
    value |= memory[address + 1];
    value <<= 8;
    value |= memory[address];
    return value;
}

halfword Memory::read_u16(word address) {
    halfword value = 0;
    value |= memory[address + 1];
    value <<= 8;
    value |= memory[address];
    return value;
}

byte Memory::read_u8(word address) {
    return memory[address];
}

void Memory::write_u32(word address, word value) {
    memory[address] = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
    memory[address + 2] = (value >> 16) & 0xFF;
    memory[address + 3] = (value >> 24) & 0xFF;
}

void Memory::write_u16(word address, halfword value) {
    memory[address] = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
}

void Memory::write_u8(word address, byte value) {
    memory[address] = value;
}

void Memory::load_rom(char *name) {
    std::ifstream rom(name, std::ios::in | std::ios::binary);
    if (!rom) return;
    size_t size = fs::file_size(name);

    if (!rom.good()) {
        std::cerr << "Bad rom!" << "\n";
        return;
    }

    rom.read((char *) memory, size);
    rom.close();
}

arm_instruction Memory::get_instruction(word address) {
    arm_instruction a = (arm_instruction) memory[address];
    arm_instruction b = (arm_instruction) memory[address + 1];
    arm_instruction c = (arm_instruction) memory[address + 2];
    arm_instruction d = (arm_instruction) memory[address + 3];

    arm_instruction i = 0;
    i |= a;
    i |= b << 8;
    i |= c << 16;
    i |= d << 24;
    return i;
}

/*
 * GBA memory can be addressed anywhere from 0x00000000-0xFFFFFFFF, however most of those addresses are unused.
 * given a 4 byte address, this function will return the address of the
 * internal region the address points to, which saves a boatload of memory.
 */
void *Memory::get_internal_region(u32 address) {
    if (address <= MEM_BIOS_END) return &memory.bios[address];
    else if (address >= MEM_BOARD_WRAM_START && address <= MEM_CHIP_WRAM_END) return &memory.board_wram[address - MEM_BOARD_WRAM_START];
    else if (address >= MEM_CHIP_WRAM_START && address <= MEM_CHIP_WRAM_END) return &memory.chip_wram[address - MEM_CHIP_WRAM_START];
    else if (address >= MEM_IO_REG_START && address <= MEM_IO_REG_END) return &memory.io_reg[address - MEM_IO_REG_START];
    else if (address >= MEM_PALETTE_RAM_START && address <= MEM_PALLETTE_RAM_END) return &memory.palette_ram[address - MEM_PALETTE_RAM_START];
    else if (address >= MEM_VRAM_START && address <= MEM_VRAM_END) return &memory.vram[address - MEM_VRAM_START];
    else if (address >= MEM_OAM_START && address <= MEM_OAM_END) return &memory.oam[address - MEM_VRAM_START];
    else {
        std::cerr << "Error: invalid internal address specified: " << address << "\n";
        exit(2);
    }
}
