#include "SRAM.h"
#include "log.h"

#include <cassert>
#include <sys/mman.h>
#include <unistd.h>

SRAM::SRAM(int size) :
    Backup(size)
{
    assert(size == 0x8000);

    cart_ram.resize(0);

    // mmap ram to backup file
    FILE *backup = fopen("sram.sav", "rwb+");

    if (!backup)
    {
        LOG(LogLevel::Error, "Unable to open SRAM backup\n");
        return;
    }

    int fd = fileno(backup);

    ram = (u8 *) mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

SRAM::~SRAM()
{
    LOG("UNAMMPING\n");
    
    // unmap backup file
    munmap(ram, size);
}

void SRAM::write(u32 index, u8 value)
{
    assert(index < size);
    ram[index] = value;
}

u8 SRAM::read(u32 index)
{
    assert(index < size);
    return ram[index];
}
