#pragma once

#include "Backup.h"

class None : public Backup
{
public:
    None(int);
    ~None() = default;

    virtual void write(u32, u8);
    virtual u8 read(u32);
};