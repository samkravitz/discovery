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

void *Memory::get_normalized_address(u32 address) {
    if (address <= 0x3FFF) {
        return &memory.bios[address];
    } else if (address >= 0x2000000 && address <= 0x0203FFFF) {
        return &memory.board_wram[address - 0x2000000];
    } else if (address >= 0x3000000 && address <= 0x03007FFF) {
        return &memory.chip_wram[address - 0x]
    }
}
