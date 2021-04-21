#pragma once

#include "Backup.h"

class None : public Backup
{
public:
    None(int);
    ~None() = default;

    virtual void write(int, u8);
    virtual u8 read(int);
};