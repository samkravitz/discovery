#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <stdint.h>
#include "../cpu/common.h"

class Memory {
    public:
        Memory();
        ~Memory();

        void load_rom(char *);
        arm_instruction get_instruction(word);
        uint8_t *get_memory() { return memory; }
    
    private:
        uint8_t *memory;
        std::size_t size;
};

#endif // MEMORY_H