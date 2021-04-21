#pragma once

#include "Backup.h"

class SRAM : public Backup
{
public:
    SRAM(int);
    ~SRAM() = default;

    void write(int, u8);
    u8 read(int);

    void loadChip();
    void writeChip();
};