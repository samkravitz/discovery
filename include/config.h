/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: config.h
 * DATE: December 7th, 2020
 * DESCRIPTION: global configuration data for discovery
 */

#pragma once

#include <SDL2/SDL.h>
#include <string>

struct Keymap 
{
    SDL_Scancode gba_a;
    SDL_Scancode gba_b;
    SDL_Scancode gba_sel;
    SDL_Scancode gba_start;
    SDL_Scancode gba_dpad_right;
    SDL_Scancode gba_dpad_left;
    SDL_Scancode gba_dpad_up;
    SDL_Scancode gba_dpad_down;
    SDL_Scancode gba_r;
    SDL_Scancode gba_l;
};

namespace config
{
    extern std::string rom_name;
    extern std::string backup_path;
    extern std::string bios_name;
    extern bool show_help;
    extern bool debug; 
    extern double framerate;

    // handle config file
    extern std::string config_file;
    void read_config_file(void);

    extern struct Keymap *keymap;
};

