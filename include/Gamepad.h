/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Gamepad.h
 * DATE: January 6th, 2021
 * DESCRIPTION: Gamepad class
 */

#pragma once

#include <SDL2/SDL.h>
#include "common.h"

class Gamepad
{

    public:
        Gamepad()  { keys.raw = 0x3FF; } // all keys released
        ~Gamepad() { }

        struct Keys
        {
            union
            {
                struct 
                {
                    u8 a     : 1;
                    u8 b     : 1;
                    u8 sel   : 1;
                    u8 start : 1;
                    u8 right : 1;
                    u8 left  : 1;
                    u8 up    : 1;
                    u8 down  : 1;
                    u8 r     : 1;
                    u8 l     : 1;
                    u8 blank : 6;
                };
                
                u16 raw;
            };
            
        } keys;

        void poll();
        void print();
};