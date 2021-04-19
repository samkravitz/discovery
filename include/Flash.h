#pragma once

#include "Backup.h"

class Flash : public Backup
{
public:
    Flash(int);
    ~Flash() = default;

    virtual void write(u32, u8);
    virtual u8 read(u32);

private:
    enum FlashState
    {
        // Prepare for command states
        READY,
        CMD_1,
        CMD_2,

        // command states
        CHIP_ID,
        PREPARE_TO_ERASE,
        ERASE_ENTIRE,
        ERASE_4K,
        PREPARE_TO_WRITE,
        SET_MEMORY_BANK,
    } state;

    enum FlashSize
    {
        SIZE_64K,
        SIZE_128K
    } flash_size;
};