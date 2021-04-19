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
#include "IRQ.h"

// global IRQ handler
IRQ *irq;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LOG(LogLevel::Error, "Error: No ROM file given\n");
        LOG("Usage: ./discovery /path/to/rom\n");
        return 1;
    }

    Discovery emulator;

    // collect command line args
    for (int i = 1; i < argc; ++i)
        emulator.argv.push_back(argv[i]);

    // parse command line args
    emulator.parseArgs();
	if(config::show_help)
	{
		emulator.printArgHelp();
		return 0;
	}

	LOG("Welcome to Discovery!\n");

    // load bios, rom, and launch game loop
    emulator.mem->loadBios(config::bios_name);
    emulator.mem->loadRom(config::rom_name);
    emulator.gameLoop();

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
    //apu     = new APU(mem);

    irq     = new IRQ();
}

void Discovery::gameLoop()
{
    u32 old_cycles = 0;
    while (running)
    {
        // tick hardware (not cpu) if in halt state
        if (mem->haltcnt)
        {   
            while ((irq->getIE() & irq->getIF()) == 0)
                tick();

            mem->haltcnt = 0;
            cpu->handleInterrupt();
        }

        cpu->fetch();
        cpu->decode(cpu->pipeline[0]);
        cpu->execute(cpu->pipeline[0]);

        cpu->handleInterrupt();

        // update pipeline
        cpu->pipeline[0] = cpu->pipeline[1];
        cpu->pipeline[1] = cpu->pipeline[2];

        // run hardware for as many clock cycles as cpu used
        old_cycles = cpu->cycles;
        while (system_cycles < old_cycles)
            this->tick();
    }

    shutdown();
}

// clock hardware components
void Discovery::tick()
{
    system_cycles++;
    ppu->tick();
    timer->tick();

    // poll for key presses at start of vblank
    if (system_cycles % 197120 == 0)
    {
        while (SDL_PollEvent(&e))
        {
            // Click X on window
            if (e.type == SDL_QUIT)
                running = false;
            
            // Press Escape key
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = false;
        }

        gamepad->poll();
    }
}

// parse command line args
void Discovery::parseArgs()
{
    for (int i = 0; i < argv.size(); ++i)
    {
        // ROM name
        if (i == 0 && util::pathExists(argv[i]))
            config::rom_name = argv[i];
		else if ((argv[i] == "-i" || argv[i] == "--input") && i != argv.size() -1)
			config::rom_name = argv[++i];
        else if ((argv[i] == "-b" || argv[i] == "--bios") && i != argv.size() - 1)
            config::bios_name = argv[++i];
		else if ((argv[i] == "-h" || argv[i] == "--help") && i == 0)
			config::show_help = true;
    }
}

void Discovery::printArgHelp()
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

void Discovery::shutdown()
{
    // free resources and shutdown
	delete cpu;
    delete ppu;
    //delete apu;
    delete mem;
    delete stat;
    delete gamepad;
    delete timer;
    delete irq;
}
