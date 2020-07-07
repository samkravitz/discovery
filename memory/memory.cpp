#include "memory.h"

#include <fstream>
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

    rom.read((char *) memory, size);
}

arm_instruction Memory::get_instruction(word address) {
    return (arm_instruction) memory[address];
}

