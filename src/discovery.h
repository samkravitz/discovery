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
        arm_7tdmi cpu;
        GPU gpu;
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

        discovery();

        void game_loop();
        void run_asm(char *);
        void poll_event();
};

#endif // DISCOVERY_H