#include "Flash.h"
#include "log.h"
#include "config.h"

#include <fstream>
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

    prepare_to_erase = false;
    chip_id_mode = false;
    bank = 0;
    state = READY;
}

void Flash::write(u32 index, u8 value)
{
    //LOG("WRITING: {:x} {:x} state={} bank={}\n", (int) value, (int) index, state, bank);

    // First check for single write
    if (state == PREPARE_TO_WRITE)
    {
        cart_ram[index + bank * 0x10000] = value;
        state = READY;
        return;
    }

    // Erase 4K sector
    if (prepare_to_erase && value == 0x30)
    {        
        u8 n = index >> 12 & 0xF; // page to be erased
        int i = n * 0x1000;        // index of first erased cell

        //LOG("ERASING SECTOR: {:x} {} bank={} {} {}\n", (int) n, i, bank, (int) (i + bank * 0x10000), prepare_to_erase);
        
        std::memset(&cart_ram[i + bank * 0x10000], 0xFF, 0x1000);
        prepare_to_erase = false;
        state = READY;
        return;
    }

    // Set memory bank
    if (state == SET_MEMORY_BANK && index == 0)
    {
        if (value == 0)
            bank = 0;
        else
            bank = 1;

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
                {
                    chip_id_mode = true;
                    state = READY;
                }
                break;
            
            case 0xF0: // Exit "Chip identification mode"
                if (state == CMD_2)
                {
                    chip_id_mode = false;
                    state = READY;
                }
                break;
            
            case 0x80: // Prepare to receive erase command
                if (state == CMD_2)
                {
                    prepare_to_erase = true;
                    state = READY;
                }
                break;
            
            case 0x10: // Erase entire chip
                if (state == CMD_2 && prepare_to_erase)
                {
                    std::memset(&cart_ram[0], 0xFF, size);
                    prepare_to_erase = false;
                    state = READY;
                }
                break;
            
            case 0x30: // Erase 4K sector
                if (state == CMD_2 && prepare_to_erase)
                {
                    LOG(LogLevel::Error, "Got to wrong Flash Erase4K\n");
                    state = READY;
                }
                break;
            
            case 0xA0: // Prepare to write single data byte
                if (state == CMD_2)
                    state = PREPARE_TO_WRITE;
                break;
            
            case 0xB0: // Set memory bank
                if (state == CMD_2 && flash_size == SIZE_128K)
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
    if (index == 0 && chip_id_mode)
    {
        switch (flash_size)
        {
            case SIZE_64K:  return 0x32;
            case SIZE_128K: return 0x62;
        }
    }

    // read device ID
    if (index == 1 && chip_id_mode)
    {
        switch (flash_size)
        {
            case SIZE_64K:  return 0x1B;
            case SIZE_128K: return 0x13;
        }
    }

    return cart_ram[index + bank * 0x10000];
}

// dump contents of cart_ram to backup .sav file
void Flash::writeChip()
{
    std::ofstream backup(config::backup_path, std::ios::out | std::ios::binary);
    assert(backup);

    backup.write((char *) &cart_ram[0], size);
    backup.close();
    LOG("Wrote save to file {}\n", config::backup_path);
}

// load contents of backup .sav file to cart_ram
void Flash::loadChip()
{
    std::ifstream backup(config::backup_path, std::ios::in | std::ios::binary);
    assert(backup);

    backup.read((char *) &cart_ram[0], size);
    backup.close();
}