#include "memory.h"

#include <fstream>
#include <iostream>
#include <sys/stat.h>

Memory::Memory() {}

Memory::~Memory() {
    delete memory;
}

void Memory::load_rom(char *name) {
    struct stat results;
    
    if (stat(name, &results) != 0) return;
    memory = new uint8_t[results.st_size];
    size = results.st_size;

    std::ifstream rom(name, std::ios::in | std::ios::binary);
    if (!rom) return;

    for (int i = 0; i < size; ++i) {
        rom.read((char *) &memory[i], sizeof(uint8_t));
        std::cout << std::hex << (int) memory[i] << std::endl;
    }
    
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
