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
#include "Util.h"

int main(int argc, char **argv)
{
    Discovery emulator;

    if (argc < 2)
    {
        LOG(LogLevel::Error, "Error: No ROM file given\n");
        LOG("Usage: ./discovery /path/to/rom\n");
        exit(1);
    }

    // collect command line args
    for (int i = 1; i < argc; ++i)
    {
        emulator.argv.push_back(argv[i]);
    }

    // parse command line args
    emulator.ParseArgs();
	if(config::show_help)
	{
		emulator.PrintArgHelp();
		return 0;
	}
	else
	{
		LOG("Welcome to Discovery!\n");
	}


    // load bios, rom, and launch game loop
    emulator.mem->LoadBios(config::bios_name);
    emulator.mem->LoadRom(config::rom_name);

    emulator.GameLoop();
    return 0;
}

Discovery::Discovery()
{
    system_cycles = 0;
    running = true;

    stat    = new LcdStat();
    mem     = new Memory(stat);

    cpu     = new Arm7Tdmi(mem);
    ppu     = new PPU(mem, stat);
    apu     = new APU(mem);
    gamepad = new Gamepad();

    

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
    u32 old_cycles = 0;

    while (running)
    {
        // tick hardware (not cpu) if in halt state
        while (mem->haltcnt)
        {
            Tick();

            auto interrupts_enabled   = mem->Read16Unsafe(REG_IE);
            auto interrupts_requested = mem->Read16Unsafe(REG_IF);

            if (interrupts_enabled & interrupts_requested != 0)
            {
                mem->haltcnt = 0;
            }
        }

        cpu->Fetch();
        cpu->Decode(cpu->pipeline[0]);
        cpu->Execute(cpu->pipeline[0]);

        cpu->HandleInterrupt();

        // update pipeline
        cpu->pipeline[0] = cpu->pipeline[1];
        cpu->pipeline[1] = cpu->pipeline[2];

        // run hardware for as many clock cycles as cpu used
        system_cycles = cpu->cycles;
        while (old_cycles++ < system_cycles)
            Tick();
    }

    ShutDown();
}

// clock hardware components
void Discovery::Tick()
{
    ppu->Tick();
    apu->GenerateChannel1();

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
        if (system_cycles % timers[j]->actual_freq == 0)
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

    // poll for key presses at start of vblank
    if (stat->scanline == VDRAW && SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            running = false;
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            mem->Write32Unsafe(REG_KEYINPUT, gamepad->Poll(e));
    }
}

// parse command line args
void Discovery::ParseArgs()
{
    for (int i = 0; i < argv.size(); ++i)
    {
        // ROM name
        if (i == 0 && Util::PathExists(argv[i]))
            config::rom_name = argv[i];
		else if ((argv[i] == "-i" || argv[i] == "--input") && i != argv.size() -1)
			config::rom_name = argv[++i];
        else if ((argv[i] == "-b" || argv[i] == "--bios") && i != argv.size() - 1)
            config::bios_name = argv[++i];
		else if ((argv[i] == "-h" || argv[i] == "--help") && i == 0)
			config::show_help = true;
    }
}

void Discovery::PrintArgHelp()
{
	LOG("Usage:\n");
	LOG("./discovery ./path/to/rom.gba\n");
	LOG("\n");
	LOG("Flags:\n");
	LOG("-i, --input\n");
	LOG("  Specifies input file for rom\n");
	LOG("-b, --bios\n");
	LOG("  Specifies GBA bios file\n");
	LOG("-h, --help\n");
	LOG("  Show help...\n");
}

void Discovery::ShutDown()
{
    // free resources and shutdown
	delete cpu;
    delete ppu;
    delete mem;
    delete stat;
    delete gamepad;

    for (int i = 0; i < 4; ++i)
        delete timers[i];
}
