/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: discovery.h
 * DATE: July 13, 2020
 * DESCRIPTION: Emulator class declaration
 */
#ifndef DISCOVERY_H
#define DISCOVERY_H

#include "arm_7tdmi.h"
#include "gpu.h"
#include "memory.h"

class discovery {
    public:
        discovery();

        arm_7tdmi cpu;
        //GPU gpu;
        Memory *mem;

        struct gamepad {
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
        } gamepad;

        void run_asm(char *);

    private:
        void game_loop();
        void poll_keys(SDL_Event);
        void shutdown();
};

#endif // DISCOVERY_H