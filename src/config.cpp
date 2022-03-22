/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: config.cpp
 * DATE: December 7th, 2020
 * DESCRIPTION: global configuration data for discovery
 */

#include <fstream>
#include <regex>
#include <vector>
#include <map>
#include <exception>
#include <algorithm>
#include "config.h" 
#include "Discovery.h"

std::map<std::string, SDL_Scancode> KeyboardInput = 
{
    {
        { "a", SDL_SCANCODE_A },
        { "b", SDL_SCANCODE_B },
        { "c", SDL_SCANCODE_C },
        { "d", SDL_SCANCODE_D },
        { "e", SDL_SCANCODE_E },
        { "f", SDL_SCANCODE_F },
        { "g", SDL_SCANCODE_G },
        { "h", SDL_SCANCODE_H },
        { "i", SDL_SCANCODE_I },
        { "j", SDL_SCANCODE_J },
        { "k", SDL_SCANCODE_K },
        { "l", SDL_SCANCODE_L },
        { "m", SDL_SCANCODE_M },
        { "n", SDL_SCANCODE_N },
        { "o", SDL_SCANCODE_O },
        { "p", SDL_SCANCODE_P },
        { "q", SDL_SCANCODE_Q },
        { "r", SDL_SCANCODE_R },
        { "s", SDL_SCANCODE_S },
        { "t", SDL_SCANCODE_T },
        { "u", SDL_SCANCODE_U },
        { "v", SDL_SCANCODE_V },
        { "w", SDL_SCANCODE_W },
        { "x", SDL_SCANCODE_X },
        { "y", SDL_SCANCODE_Y },
        { "z", SDL_SCANCODE_Z },
        { "cr", SDL_SCANCODE_RETURN },
        { "bs", SDL_SCANCODE_BACKSPACE },
        { "right", SDL_SCANCODE_RIGHT },
        { "left", SDL_SCANCODE_LEFT },
        { "up", SDL_SCANCODE_UP },
        { "down", SDL_SCANCODE_DOWN },
        { "space", SDL_SCANCODE_SPACE },
        { "tab", SDL_SCANCODE_TAB },
        { "rshift", SDL_SCANCODE_RSHIFT },
        { "lshift", SDL_SCANCODE_LSHIFT },
        { "rctrl", SDL_SCANCODE_RCTRL },
        { "lctrl", SDL_SCANCODE_LCTRL },
        { ",", SDL_SCANCODE_COMMA },
        { ".", SDL_SCANCODE_PERIOD }
    }
};

namespace config 
{
    // default config values
    std::string rom_name = "";
    std::string backup_path = "";
    std::string bios_name = "gba_bios.bin";
    bool show_help = false;
    bool debug = false;
    double framerate = 0.;

    // default config file
    std::string config_file = "discovery.config";

    // define default keymap
    struct Keymap *keymap = new Keymap {
        gba_a: KeyboardInput["x"],
        gba_b: KeyboardInput["z"],
        gba_sel: KeyboardInput["bs"],
        gba_start: KeyboardInput["cr"],
        gba_dpad_right: KeyboardInput["right"],
        gba_dpad_left: KeyboardInput["left"],
        gba_dpad_up: KeyboardInput["up"],
        gba_dpad_down: KeyboardInput["down"],
        gba_r: KeyboardInput["s"],
        gba_l: KeyboardInput["a"]
    };
}

void config::read_config_file()
{
    try 
    {
        std::ifstream file;
        std::map<std::string, std::string> config;
        file.open(config_file, std::ios::binary);
        
        // read lines in config file
        if(file.fail())
            throw std::runtime_error("Could not read Discovery config file.");
        if(file.is_open())
        {
            std::string line;
            std::regex reg("^\\s*([^\\W][\\w]*)\\s*=\\s*(<?[^\\W\\s][\\w]*\\S*>?)|(#{1,})+2.*$");
            std::smatch group;

            while(std::getline(file, line)) 
            {
                if(std::regex_search(line, group, reg))  
                {
                    if(group.size()) 
                        config.insert({group[1].str(), group[2].str()});
                    else
                        log(LogLevel::Error, "Error parsing config file.");
                }
            }
        }
        file.close();
        
        // validate and set key/value pair
        for(auto it = config.begin(); it != config.end(); it++)
        {
            std::string key = it->first, val = it->second;
            auto keymap_code = KeyboardInput.find(val);
            if(keymap_code != KeyboardInput.end())
            {
                if(key == "gba_a") keymap->gba_a = keymap_code->second;
                else if(key == "gba_b") keymap->gba_b = keymap_code->second;
                else if(key == "gba_sel") keymap->gba_sel = keymap_code->second;
                else if(key == "gba_start") keymap->gba_start = keymap_code->second;
                else if(key == "gba_dpad_right") keymap->gba_dpad_right = keymap_code->second;
                else if(key == "gba_dpad_left") keymap->gba_dpad_left = keymap_code->second;
                else if(key == "gba_dpad_up") keymap->gba_dpad_up = keymap_code->second;
                else if(key == "gba_dpad_down") keymap->gba_dpad_down = keymap_code->second;
                else if(key == "gba_r") keymap->gba_r = keymap_code->second;
                else if(key == "gba_r") keymap->gba_l = keymap_code->second;
            }
        }
    } 
    catch(std::exception &e)
    {
        // handle no config file
        log(LogLevel::Warning, "Could not read Discovery config file.");
    }
}

