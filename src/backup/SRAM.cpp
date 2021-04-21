#include "SRAM.h"
#include "config.h"

#include <fstream>
#include <cassert>

SRAM::SRAM(int size) :
    Backup(size)
{
    assert(size == 0x10000);
}

void SRAM::write(int index, u8 value)
{
    assert(index < size);
    cart_ram[index] = value;
}

u8 SRAM::read(int index)
{
    assert(index < size);
    return cart_ram[index];
}

// dump contents of cart_ram to backup .sav file
void SRAM::writeChip()
{
    std::ofstream backup(config::backup_path, std::ios::out | std::ios::binary);
    assert(backup);

    backup.write((char *) &cart_ram[0], size);
    backup.close();
    LOG("Wrote save to file {}\n", config::backup_path);
}

// load contents of backup .sav file to cart_ram
void SRAM::loadChip()
{
    std::ifstream backup(config::backup_path, std::ios::in | std::ios::binary);
    assert(backup);

    backup.read((char *) &cart_ram[0], size);
    backup.close();
}