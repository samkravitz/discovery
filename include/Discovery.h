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
#include "Memory.h"
#include "Timer.h"
#include "Gamepad.h"
#include "config.h"

class Discovery
{
    public:
        Discovery();

        Arm7Tdmi  *cpu;
        PPU       *ppu;
        Memory    *mem;
        LcdStat   *stat;
        Gamepad   *gamepad;
        Timer     *timers[4];

        long system_cycles;
        bool running;

        SDL_Event e;

        std::vector<std::string> argv;

        void GameLoop();
        void Tick();
        void ParseArgs();
        void PrintArgHelp();
        void ShutDown();
};