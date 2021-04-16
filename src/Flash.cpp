#include "Flash.h"

#include <cassert>

Flash::Flash(int size) :
    Backup(size)
{
    assert(size == 65536 || size == 131072);
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
    return 0;
}