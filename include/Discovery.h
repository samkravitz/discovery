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

class Discovery
{
public:
    Discovery();

    Arm7Tdmi  *cpu;
    PPU       *ppu;
    APU       *apu;
    Memory    *mem;
    LcdStat   *stat;
    Gamepad   *gamepad;
    Timer     *timer;

    void parseArgs();
    void printArgHelp();
    void gameLoop();

        std::vector<std::string> argv;
private:
    u64  cycles;
    bool running;
    SDL_Event e;
    
    void tick();
    void shutdown();
};