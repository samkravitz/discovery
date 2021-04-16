#pragma once

#include "common.h"
#include "log.h"

#include <cassert>

class Backup
{
public:
    explicit Backup(int size) :
        size(size)
    {
        backup_type = NONE;
        cart_ram = new u8[size];
    }

    ~Backup() = default;

    virtual u8 read(u32 address)
    {
        assert(address < size);

        return 0xFF;
    }

    virtual void write(u32 address, u8 value)
    {
        assert(address < size);

        cart_ram[address] = value;
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