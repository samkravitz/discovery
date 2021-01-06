/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: discovery.h
 * DATE: July 13, 2020
 * DESCRIPTION: Emulator class declaration
 */
#pragma once

#include "arm_7tdmi.h"
#include "gpu.h"
#include "memory.h"
#include "timer.h"

class Discovery
{
    public:
        Discovery();

        Arm7Tdmi  *cpu;
        GPU       *gpu;
        Memory    *mem;
        LcdStat  *stat;
        Timer     *timers[4];

        struct Gamepad
        {
            u8 a;
            u8 b;
            u8 sel;
            u8 start;
            u8 right;
            u8 left;
            u8 up;
            u8 down;
            u8 r;
            u8 l;
            u16 keys;
        } Gamepad;

        u32 system_cycles;

        void LoadRom(char *);

    private:
        void GameLoop();
        void PollKeys(const SDL_Event &);
        void ShutDown();

        void game_loop_debug();
        void print_debug_info();
};