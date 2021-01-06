/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Discovery.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of the core emulator class
 */
#include <iostream>
#include <iomanip>

#include "Discovery.h"

int main(int argc, char **argv)
{
    LOG("Welcome to Discovery!\n");

    Discovery emulator;

    // load bios
    if (!emulator.mem->LoadBios())
    {
        LOG(LogLevel::Error, "Error Loading Bios\n");
        return 1;
    }
    
    emulator.LoadRom(argv[1]);
    return 0;
}

Discovery::Discovery()
{
    system_cycles = 0;

    cpu     = new Arm7Tdmi();
    mem     = new Memory();
    ppu     = new PPU();
    gamepad = new Gamepad();

    cpu->mem = mem;
    ppu->mem = mem;

    stat = new LcdStat();
    mem->stat = stat;
    ppu->stat = stat;

    // initialize timers
    Timer *t0 = new Timer();
    Timer *t1 = new Timer();
    Timer *t2 = new Timer();
    Timer *t3 = new Timer();

    timers[0] = t0;
    timers[1] = t1;
    timers[2] = t2;
    timers[3] = t3;
    
    // link system's timers to memory's
    mem->timers[0] = timers[0];
    mem->timers[1] = timers[1];
    mem->timers[2] = timers[2];
    mem->timers[3] = timers[3];
}

void Discovery::GameLoop()
{
    SDL_Event e;
    u32 old_cycles = 0;
    int num = 0;
    bool valid;

    while (true)
    {
        cpu->Fetch();
        cpu->Decode(cpu->pipeline[0]);
        cpu->Execute(cpu->pipeline[0]);

        cpu->HandleInterrupt();

        // update pipeline
        cpu->pipeline[0] = cpu->pipeline[1];
        cpu->pipeline[1] = cpu->pipeline[2];

        // run gpu and timers for as many clock cycles as cpu used
        system_cycles = cpu->cycles;
        for (int i = system_cycles - old_cycles; i > 0; --i)
        {
            ++old_cycles;

            ppu->Tick();

            // clock timers
            for (int j = 0; j < 4; ++j)
            {
                // ignore if timer is disabled
                if (!timers[j]->enable)
                    continue;
                
                // ignore if cascade bit is set (timer will be incremented by previous timer)
                if (timers[j]->cascade)
                    continue;
                
                // increment counter by 1
                if (old_cycles % timers[j]->actual_freq == 0)
                {
                    timers[j]->data += 1; // increment timer

                    // timer overflowed
                    if (timers[j]->data == 0x0000)
                    {
                        // reset timer
                        timers[j]->data = timers[j]->start_data;

                        //std::cout << "Timer " << j << " overflow\n";

                        // overflow irq
                        if (timers[j]->irq)
                            LOG("Timer {} overflow IRQ request\n");

                        // cascade
                        // timer 4 can't cascade any other timer
                        if (j == 4)
                            continue;
                        
                        if (timers[j + 1]->enable && timers[j + 1]->cascade)
                        {
                            timers[j + 1]->data += 1;

                            // cascade caused overflow (deal with this later)
                            if (timers[j + 1]->data == 0x0000)
                                LOG("Timer {} cascade overflow\n", j + 1);
                        }
                    }
                }
            }
        }

        // poll for key presses at start of vblank
        if (stat->scanline == VDRAW && SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                break;
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
                mem->Write32Unsafe(REG_KEYINPUT, gamepad->Poll(e));
        }

        valid = false;
    }
    
    ShutDown();
}

void Discovery::LoadRom(char *name)
{
    if (!mem->LoadRom(name))
    {
        LOG(LogLevel::Error, "Error loading ROM: {}\n", name);
        exit(1);
    }

    GameLoop();
}

void Discovery::ShutDown()
{
    // free resources and shutdown
    delete mem;

    for (int i = 0; i < 4; ++i)
        delete timers[i];
        
    cpu->~Arm7Tdmi();
    ppu->~PPU();
}