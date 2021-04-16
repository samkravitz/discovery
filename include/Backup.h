#pragma once

#include "common.h"

#include <cassert>

class Backup
{
public:
    Backup(int size) :
        size(size)
    {
        backup_type = NONE;
        cart_ram = new u8[size];
    }

    ~Backup() = default;

    virtual u8 read(u32 address)
    {
        assert(address < size);

        return cart_ram[address - 0xE0000000];
    }

    virtual void write(u32 address, u8 value)
    {
        assert(address < size);

        cart_ram[address - 0xE0000000] = value;
    }

protected:
    int size;
    u8 *cart_ram;

    enum BackupType
    {
        SRAM,
        EEPROM,
        FLASH,
        NONE
    };

    BackupType backup_type;
};