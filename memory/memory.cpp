#include "memory.h"

#include <fstream>
#include <iostream>
#include <sys/stat.h>

Memory::Memory() {
    for (int i = 0; i < GBA_MEM_SIZE; ++i) memory[i] = 0;
}

Memory::~Memory() {}

word Memory::read_u32(word address) {
    word value = 0;
    value |= memory[address];
    value <<= 8;
    value |= memory[address + 1];
    value <<= 8;
    value |= memory[address + 2];
    value <<= 8;
    value |= memory[address + 3];
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

void Memory::write_u8(word address, byte value) {
    memory[address] = value;
}

void Memory::load_rom(char *name) {
    // struct stat results;
    
    // if (stat(name, &results) != 0) return;
    // memory = new uint8_t[results.st_size];
    // size = results.st_size;

    // std::ifstream rom(name, std::ios::in | std::ios::binary);
    // if (!rom) return;

    // for (int i = 0; i < size; ++i) {
    //     rom.read((char *) &memory[i], sizeof(uint8_t));
    //     std::cout << std::hex << (int) memory[i] << std::endl;
    // }
    
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

