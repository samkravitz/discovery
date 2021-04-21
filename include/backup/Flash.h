#pragma once

#include "Backup.h"

class Flash : public Backup
{
public:
    Flash(int);
    ~Flash() = default;

    void write(u32, u8);
    u8 read(u32);

    void writeChip();
    void loadChip();

private:

    // current memory bank (0 or 1)
    int bank;

    // Chip identification mode is activated
    bool chip_id_mode;

    // next command is to be an erase command
    bool prepare_to_erase;

    enum FlashState
    {
        // Prepare for command states
        READY,
        CMD_1,
        CMD_2,

        // command states
        PREPARE_TO_ERASE,
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