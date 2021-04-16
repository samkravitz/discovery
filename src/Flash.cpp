#include "Flash.h"

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

    state = READY;
}

void Flash::write(u32 address, u8 value)
{
    // switch (address)
    // {
    //     case 0xE005555:
    //         if (value == 0xAA && flash_state == READY)
    //             flash_state = CMD_1;
    //         break;
        
    //     case 0xE002AAA:
    //         if (value == 0x55 && flash_state == CMD_1)
    //             flash_state = CMD_2;

    // }
}

u8 Flash::read(u32 address)
{
    // read device ID
    if (address == 0xE000000)
    {
        switch (flash_size)
        {
            case SIZE_64K:  return 0x32;
            case SIZE_128K: return 0x62;
        }
    }

    if (address == 0xE000001)
    {
        switch (flash_size)
        {
            case SIZE_64K:  return 0x1B;
            case SIZE_128K: return 0x13;
        }
    }
    
    return 0;
}