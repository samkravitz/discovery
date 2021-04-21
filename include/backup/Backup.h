#pragma once

#include "common.h"
#include "log.h"

#include <cstring>
#include <vector>

class Backup
{
public:
    Backup(size_t size) :
        size(size)
    {
        cart_ram.resize(size);

        // uniitialized memory is 0xFF
        std::memset(&cart_ram[0], 0xFF, size);
    }

    ~Backup() = default;

    virtual u8 read(int) = 0;
    virtual void write(int, u8) = 0;
    virtual void loadChip() { }
    virtual void writeChip() { }

protected:
    std::vector<u8> cart_ram;
    std::size_t size;
};