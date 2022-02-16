#include "Discovery.h"
#include "config.h"

#include <SDL2/SDL.h>
#include <cassert>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

SDL_Window  *window;
SDL_Surface *final_screen;
SDL_Surface *original_screen;
SDL_Rect     scale_rect;

void init();

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

    init();

	log("Welcome to Discovery!\n");

    // load bios, rom, and launch game loop
    emulator.mem->loadBios(config::bios_name);
    emulator.mem->loadRom(config::rom_name);
    

    bool running = true;
    SDL_Event e;
    int frame = 0;
    clock_t old_time = std::clock();
    u32 old_ticks = SDL_GetTicks();

    while (running)
    {
        emulator.frame();

        original_screen->pixels = (u32 *) emulator.ppu->screen_buffer;
        // scale screen buffer
        SDL_BlitScaled(original_screen, nullptr, final_screen, &scale_rect);

        // draw final_screen pixels on screen
        SDL_UpdateWindowSurface(window);
        
        // calculate fps
        double fps, duration;
        u32 new_ticks;
        if (++frame == 60)
        {
            frame = 0;
            double last_dur = duration;
            clock_t new_time = std::clock();
            duration = (new_time - old_time) / (double) emulator.cpu->CLOCKS_PER_SECOND;
            new_ticks = SDL_GetTicks() - old_ticks;
            std::cout << "clocks p s: " << emulator.cpu->CLOCKS_PER_SECOND << std::endl;
            std::cout << "old ticks: " << old_ticks << std::endl;
            std::cout << "new ticks: " << new_ticks << std::endl;
            std::cout << "dur: " << duration << std::endl;
            std::cout << "1-dur: " << 1-duration << std::endl;
            std::cout << "fps: " << fps << std::endl;
            old_time = new_time;
            old_ticks = new_ticks;
            fps = 60 / (double) (1. - duration);

            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << fps;
            std::string title("");
            title += "discovery - ";
            title += stream.str();
            title += " fps";
            SDL_SetWindowTitle(window, title.c_str());
        }
            
        // cause a delay if frame rate is over 60
        if(std::round(fps) > 60) {
            u16 real_time_diff_ms = (u16) std::round(10 - ((1 - duration) * 10));
            u16 delay = ((u16) std::ceil(std::abs(duration - (new_ticks / old_ticks)))) + real_time_diff_ms;
            // std::cout << "real time diff: " << real_time_diff_ms << ", delay: " << delay << std::endl;
            SDL_Delay(delay);
        } else {
          std::cout << "fps <= 60, won't delay\n";
        }

        while (SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_QUIT:
                    // Click X on window
                    running = false;
                    break;
                case SDL_KEYDOWN: [[fallthrough]];
                case SDL_KEYUP:
                    // Key press event
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                        running = false;                     
                    emulator.gamepad->poll();
                    break;
                case SDL_WINDOWEVENT:
                    if(e.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        int width = e.window.data1, height = e.window.data2;
                        float real_rat = (float) width / (float) height;
                        float gbas_rat = (float) SCREEN_WIDTH / (float) SCREEN_HEIGHT;
                        float prop = real_rat / gbas_rat;
                        float ratw = prop > 1.0f ? height*gbas_rat : width;
                        float rath = prop > 1.0f ? height : height*prop;
                        float cx = prop > 1.0f ? (width-ratw)/2 : 0;
                        float cy = prop > 1.0f ? 0: (height-rath)/2;
                        SDL_SetWindowSize(window, width, height);
                        final_screen = SDL_GetWindowSurface(window);
                        scale_rect.w = ratw;
                        scale_rect.h = rath;
                        scale_rect.x = cx;
                        scale_rect.y = cy;
                        SDL_BlitScaled(original_screen, nullptr, final_screen, &scale_rect);
                    }
            }
        }
    }

    emulator.shutdown();

    SDL_Quit();

    return 0;
}

void init()
{
    assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

    window = SDL_CreateWindow(
        "discovery", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        SCREEN_WIDTH * 2, 
        SCREEN_HEIGHT * 2, 
        SDL_WINDOW_RESIZABLE);
    assert(window);

    final_screen = SDL_GetWindowSurface(window);
    original_screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    
    scale_rect.w = SCREEN_WIDTH  * 2;
    scale_rect.h = SCREEN_HEIGHT * 2;
    scale_rect.x = 0;
    scale_rect.y = 0;
}

