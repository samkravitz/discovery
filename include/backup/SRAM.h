#pragma once

#include "Backup.h"

class SRAM : public Backup
{
public:
    SRAM(int);
    ~SRAM();

    virtual void write(u32, u8);
    virtual u8 read(u32);

private:
    //u8 *ram;
};