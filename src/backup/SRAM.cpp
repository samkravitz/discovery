#include "SRAM.h"

#include <cassert>

SRAM::SRAM(int size) :
    Backup(size)
{
    assert(size == 0x10000);
}

void SRAM::write(u32 index, u8 value)
{
    assert(index < size);
    cart_ram[index] = value;
}

u8 SRAM::read(u32 index)
{
    assert(index < size);
    return cart_ram[index];
}
