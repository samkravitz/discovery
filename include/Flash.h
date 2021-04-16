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
        READY,
        CMD_1,
        CMD_2,
    } state;

    enum FlashSize
    {
        SIZE_64K,
        SIZE_128K
    } flash_size;
};