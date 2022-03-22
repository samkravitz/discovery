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
#include "IRQ.h"
#include "config.h"

extern IRQ *irq;

// get current key state
void Gamepad::poll()
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

    //SDL_PumpEvents();
    auto *state = SDL_GetKeyboardState(nullptr);

    keys.a     = state[config::keymap->gba_a]              ? 0 : 1;
    keys.b     = state[config::keymap->gba_b]              ? 0 : 1;
    keys.sel   = state[config::keymap->gba_sel]            ? 0 : 1;
    keys.start = state[config::keymap->gba_start]          ? 0 : 1;
    keys.right = state[config::keymap->gba_dpad_right]     ? 0 : 1;
    keys.left  = state[config::keymap->gba_dpad_left]      ? 0 : 1;
    keys.up    = state[config::keymap->gba_dpad_up]        ? 0 : 1;
    keys.down  = state[config::keymap->gba_dpad_down]      ? 0 : 1;
    keys.r     = state[config::keymap->gba_r]              ? 0 : 1;
    keys.l     = state[config::keymap->gba_l]              ? 0 : 1;

    if (keycnt.irq) // key interrupts enabled
        checkInterrupt();
}

void Gamepad::checkInterrupt()
{
    // zero blank bits in keys just in case they're not
    keys.blank = 0;
    int keys_to_check = keycnt.raw & 0x3FF;
    bool raise_interrupt = false; // interrupt condition

    if (keycnt.condition == 1) // use AND (raise if all keys are down)
    {
        raise_interrupt = true;
        for (int i = 0; i < 10; ++i) // 10 keys
        {
            if ((keys_to_check >> i & 1) && (keys.raw >> i & 1))
            {
                raise_interrupt = false;
                break;
            }
        }
    }

    else // use OR (raise if any keys are down)
    {
        for (int i = 0; i < 10; ++i) // 10 keys
        {
            if ((keys_to_check >> i & 1) && (keys.raw >> i & 1) == 0)
            {
                raise_interrupt = true;
                break;
            }
        }
    }

    // raise keypad interrupt
    if (raise_interrupt)
    {
        log("Raising gamepad interrupt\n");
        irq->raise(InterruptOccasion::KEYPAD);
    }
}
