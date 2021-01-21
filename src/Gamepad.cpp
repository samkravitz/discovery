/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 *
 * FILE: Gamepad.cpp
 * DATE: January 6th, 2021
 * DESCRIPTION: Gamepad class
 */
#include "Gamepad.h"

// get current key state
u16 Gamepad::Poll(const SDL_Event &e)
{
    /*
     * Order of keys in KEYINPUT is as follows:
     * a: 0
     * b: 1
     * select: 2
     * start: 3
     * right: 4
     * left: 5
     * up: 6
     * down: 7
     * r: 8
     * l: 9
     */
    // poll button presses
    if (e.type == SDL_KEYDOWN)
    {
        switch(e.key.keysym.sym)
        {
            case SDLK_x:         keys.a     = 0; break;
            case SDLK_z:         keys.b     = 0; break;
            case SDLK_BACKSPACE: keys.sel   = 0; break;
            case SDLK_RETURN:    keys.start = 0; break;
            case SDLK_RIGHT:     keys.right = 0; break;
            case SDLK_LEFT:      keys.left  = 0; break;
            case SDLK_UP:        keys.up    = 0; break;
            case SDLK_DOWN:      keys.down  = 0; break;
            case SDLK_s:         keys.r     = 0; break;
            case SDLK_a:         keys.l     = 0; break;
            case SDLK_ESCAPE:    exit(0); // escape
        }
    }

    // poll button releases
    if (e.type == SDL_KEYUP)
    {
        switch(e.key.keysym.sym)
        {
            case SDLK_x:         keys.a     = 1; break;
            case SDLK_z:         keys.b     = 1; break;
            case SDLK_BACKSPACE: keys.sel   = 1; break;
            case SDLK_RETURN:    keys.start = 1; break;
            case SDLK_RIGHT:     keys.right = 1; break;
            case SDLK_LEFT:      keys.left  = 1; break;
            case SDLK_UP:        keys.up    = 1; break;
            case SDLK_DOWN:      keys.down  = 1; break;
            case SDLK_s:         keys.r     = 1; break;
            case SDLK_a:         keys.l     = 1; break;
        }
    }

    return keys.raw;

    // // check for key interrupt
    // u16 keycnt = mem->Read16Unsafe(REG_KEYCNT);
    // if (keycnt >> 14 & 0x1) // key interrupts enabled
    // {
    //     u16 keys = keycnt & 0x3FF; // keys to check

    //     bool raise_interrupt = false;
    //     if (keycnt >> 15) // use AND (raise if all keys are down)
    //     {
    //         // all keys to check are down (high)
    //         if (keys == ~gamepad_result)
    //             raise_interrupt = true;
    //     }

    //     else // use OR (raise if any keys are down)
    //     {
    //         for (int i = 0; i < 10; ++i) // 10 keys
    //         {
    //             if ((keys >> i & 1) && (gamepad_result >> i & 1) == 0)
    //             {
    //                 raise_interrupt = true;
    //                 break;
    //             }
    //         }
    //     }

    //     // raise keypad interrupt
    //     if (raise_interrupt)
    //     {
    //         std::cout << "Raising gamepad interrupt\n";
    //         u16 reg_if = mem->Read16Unsafe(REG_IF) | IRQ_KEYPAD;
    //         mem->Write16Unsafe(REG_IF, reg_if);
    //     }
    // }

    // // store gamepad result back into the KEYINPUT address
    // mem->Write32Unsafe(REG_KEYINPUT, gamepad_result);
}

void Gamepad::Print()
{
    LOG("\n\n");
    if (keys.l     == 0) LOG("L is pressed\n");
    if (keys.r     == 0) LOG("R is pressed\n");
    if (keys.down  == 0) LOG("Down is pressed\n");
    if (keys.up    == 0) LOG("Up is pressed\n");
    if (keys.left  == 0) LOG("Left is pressed\n");
    if (keys.right == 0) LOG("Right is pressed\n");
    if (keys.start == 0) LOG("Start is pressed\n");
    if (keys.sel   == 0) LOG("Select is pressed\n");
    if (keys.b     == 0) LOG("b is pressed\n");
    if (keys.a     == 0) LOG("a is pressed\n");
}
