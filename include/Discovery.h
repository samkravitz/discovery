/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Discovery.h
 * DATE: July 13, 2020
 * DESCRIPTION: Emulator class declaration
 */
#pragma once

#include <vector>

#include "Arm7Tdmi.h"
#include "PPU.h"
#include "APU.h"
#include "Memory.h"
#include "Timer.h"
#include "Gamepad.h"
#include "Scheduler.h"

class Discovery
{
public:
    Discovery();

    Arm7Tdmi  *cpu;
    PPU       *ppu;
    APU       *apu;
    Memory    *mem;
    LcdStat   *lcdStat;
    Gamepad   *gamepad;
    Timer     *timer;
    Scheduler *scheduler;

    std::vector<std::string> argv;

    void frame();
    void parseArgs();
    void printArgHelp();
    void shutdown();
};