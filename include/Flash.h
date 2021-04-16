#pragma once

#include "Backup.h"

class Flash : public Backup
{
public:
    Flash(int);
    ~Flash() = default;

    virtual void write(u32, u8);
    virtual u8 read(u32);
};