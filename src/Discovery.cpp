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
#include "config.h"

// global IRQ handler
IRQ *irq;

Discovery::Discovery()
{
    gamepad   = new Gamepad();
    stat      = new LcdStat();
    scheduler = new Scheduler();
    timer     = new Timer(scheduler);

    mem       = new Memory(stat, timer, gamepad);
    cpu       = new Arm7Tdmi(mem);
    ppu       = new PPU(mem, stat, scheduler);
    //apu     = new APU(mem);
    irq       = new IRQ();
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
    delete scheduler;
}

void Discovery::frame()
{
    int cycles = 0;
    int cycles_elapsed;

    while (cycles < 280896)
    {
        // tick hardware (not cpu) if in halt state
        if (mem->haltcnt)
        {   
            LOG("HALT\n");
            
            while ((irq->getIE() & irq->getIF()) == 0)

            mem->haltcnt = 0;
            cpu->handleInterrupt();
            LOG("UNHALT\n");
            
        }

        cpu->fetch();
        cpu->decode();
        cycles_elapsed = cpu->execute(cpu->pipeline[0]);
        scheduler->advance(cycles_elapsed);

        cpu->handleInterrupt();

        // update pipeline
        cpu->pipeline[0] = cpu->pipeline[1];
        cpu->pipeline[1] = cpu->pipeline[2];

        // run hardware for as many clock cycles as cpu used
        while (cycles_elapsed-- > 0)
            ++cycles;
    }

}
