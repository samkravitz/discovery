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
#include "util.h"

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

    gamepad = new Gamepad();
    stat    = new LcdStat();
    timer   = new Timer();

    mem     = new Memory(stat, timer, gamepad);
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
                mem->haltcnt = 0;
        }

        cpu->Fetch();
        cpu->Decode(cpu->pipeline[0]);
        cpu->Execute(cpu->pipeline[0]);

        cpu->HandleInterrupt();

        // update pipeline
        cpu->pipeline[0] = cpu->pipeline[1];
        cpu->pipeline[1] = cpu->pipeline[2];

        // run hardware for as many clock cycles as cpu used
        old_cycles = cpu->cycles;
        while (system_cycles < old_cycles)
            Tick();
    }

    ShutDown();
}

// clock hardware components
void Discovery::Tick()
{
    system_cycles++;
    ppu->Tick();
    timer->Tick();

    // poll for key presses at start of vblank
    if (system_cycles % 197120 == 0)
    {
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT)
           running = false;

        gamepad->Poll();
    }
}

// parse command line args
void Discovery::ParseArgs()
{
    for (int i = 0; i < argv.size(); ++i)
    {
        // ROM name
        if (i == 0 && util::PathExists(argv[i]))
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
    delete timer;
}
