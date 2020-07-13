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

        uint8_t *memory;
        
        void load_rom(char *);
        arm_instruction get_instruction(word);
        uint8_t *get_memory() { return memory; }

        // read / write from memory
        word read_u32(word);
        halfword read_u16(word);
        byte read_u8(word);
        void write_u8(word, byte);
        void write_u16(word, halfword);
        void write_u32(word, word);

    private:
        std::size_t size;
};

#endif // MEMORY_H