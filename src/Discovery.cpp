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
    lcdStat   = new LcdStat();
    scheduler = new Scheduler();
    timer     = new Timer(scheduler);

    mem       = new Memory(lcdStat, timer, gamepad);
    cpu       = new Arm7Tdmi(mem);
    ppu       = new PPU(mem, lcdStat, scheduler);
    apu       = new APU(mem, scheduler);
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
	log("Usage:\n");
	log("./discovery ./path/to/rom.gba\n");
	log("\n");
	log("Flags:\n");
	log("-i, --input\n");
	log("  Specifies input file for rom\n");
	log("-b, --bios\n");
	log("  Specifies GBA bios file\n");
	log("-h, --help\n");
	log("  Show help...\n");
}

void Discovery::shutdown()
{
    // free resources and shutdown
	delete cpu;
    delete ppu;
    delete apu;
    delete mem;
    delete lcdStat;
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
            while ((irq->getIE() & irq->getIF()) == 0)

            mem->haltcnt = 0;
            cpu->handleInterrupt();
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
