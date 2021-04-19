#pragma once

#include "common.h"
#include "log.h"

#include <cstring>
#include <vector>

class Backup
{
public:
    Backup(int size) :
        size(size)
    {
        cart_ram.resize(size);

        // uniitialized memory is 0xFF
        std::memset(&cart_ram[0], 0xFF, size);
    }

    ~Backup() = default;

    virtual u8 read(u32) = 0;
    virtual void write(u32, u8) = 0;

protected:
    std::vector<u8> cart_ram;
    std::size_t size;
};