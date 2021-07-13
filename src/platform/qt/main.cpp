#include <cassert>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <SDL2/SDL.h>

#include <QApplication>
#include <QWidget>

#include "config.h"
#include "DebugMainWindow.h"
#include "Discovery.h"

SDL_Window  *window;
SDL_Surface *final_screen;
SDL_Surface *original_screen;
SDL_Rect     scale_rect;

void init();

void frame(Discovery const &emulator)
{
    emulator.frame();

    original_screen->pixels = (u32 *) emulator.ppu->screen_buffer;

    // scale screen buffer
    SDL_BlitScaled(original_screen, nullptr, final_screen, &scale_rect);

    // draw final_screen pixels on screen
    SDL_UpdateWindowSurface(window);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        log(LogLevel::Error, "Error: No ROM file given\n");
        log("Usage: ./discovery /path/to/rom\n");
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

	log("Welcome to Discovery!\n");

    init();

    // load bios, rom, and launch game loop
    emulator.mem->loadBios(config::bios_name);
    emulator.mem->loadRom(config::rom_name);

    QApplication app(argc, argv);
    DebugMainWindow dmw(emulator);
    dmw.show();
    return app.exec();
}

void init()
{
    assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

    window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, 0);
    assert(window);

    final_screen = SDL_GetWindowSurface(window);
    original_screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    
    scale_rect.w = SCREEN_WIDTH  * 2;
    scale_rect.h = SCREEN_HEIGHT * 2;
    scale_rect.x = 0;
    scale_rect.y = 0;
}