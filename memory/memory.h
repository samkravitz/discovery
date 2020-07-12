#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <stdint.h>
#include "common.h"
#include "../cpu/common.h"

class Memory {
    public:
        Memory();
        ~Memory();

        uint8_t memory[GBA_MEM_SIZE];
        
        void load_rom(char *);
        arm_instruction get_instruction(word);
        uint8_t *get_memory() { return memory; }

        word read_u32(word);
        halfword read_u16(word);
        byte read_u8(word);
    private:
        std::size_t size;
};

#endif // MEMORY_H