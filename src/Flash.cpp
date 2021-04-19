#include "Flash.h"
#include "log.h"

#include <cstring>
#include <cassert>

Flash::Flash(int size) :
    Backup(size)
{
    assert(size == 65536 || size == 131072);

    switch (size)
    {
        case 65536:  flash_size = SIZE_64K;  break;
        case 131072: flash_size = SIZE_128K; break;
    }

    // uniitialized memory is 0xFF
    std::memset(cart_ram, 0xFF, size);

    state = READY;
}

void Flash::write(u32 index, u8 value)
{
    // First check for single write
    if (state == PREPARE_TO_WRITE)
    {
        LOG("WRITING\n");
        // TODO - add memory bank
        cart_ram[index] = value;
        state = READY;
        return;
    }

    // Erase 4K sector
    if (state == ERASE_4K && value == 0x30)
    {
        u8 n = index >> 12 & 0xF; // page to be erased
        int i = n * 0x400;
        std::memset(&cart_ram[i], 0xFF, 0x1000);
        state = READY;
        return;
    }

    // Check for change command
    if (index == 0x5555)
    {
        switch (value)
        {
            case 0xAA:
                if (state == READY)
                    state = CMD_1;
                break;

            case 0x90: // Enter "Chip identification mode"
                if (state == CMD_2)
                    state = CHIP_ID;
                break;
            
            case 0xF0: // Exit "Chip identification mode"
                if (state == CHIP_ID)
                    state = READY;
                break;
            
            case 0x80: // Prepare to receive erase command
                if (state == CMD_2)
                    state = PREPARE_TO_ERASE;
                break;
            
            case 0x10: // Erase entire chip
                if (state == PREPARE_TO_ERASE)
                {
                    std::memset(cart_ram, 0xFF, size);
                    state = READY;
                }
                break;
            
            case 0x30: // Erase 4K sector
                if (state == PREPARE_TO_ERASE)
                {
                    ;
                    state = READY;
                }
                break;
            
            case 0xA0: // Prepare to write single data byte
                if (state == CMD_2)
                    state = PREPARE_TO_WRITE;
                break;
            
            case 0xB0: // Set memory bank
                if (flash_size == SIZE_128K && state == CMD_2)
                    state = SET_MEMORY_BANK;
                break;
            
            default: // reset to ready
                state = READY;
        }
    }
        
    else if (index == 0x2AAA)
    {
        if (value == 0x55 && state == CMD_1)
                state = CMD_2;
    }

}

u8 Flash::read(u32 index)
{
    // read device ID
    if (index == 0 && state == CHIP_ID)
    {
        switch (flash_size)
        {
            case SIZE_64K:  return 0x32;
            case SIZE_128K: return 0x62;
        }
    }

    // read device ID
    if (index == 1 && state == CHIP_ID)
    {
        switch (flash_size)
        {
            case SIZE_64K:  return 0x1B;
            case SIZE_128K: return 0x13;
        }
    }
    
    return cart_ram[index];
}