/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: config.h
 * DATE: December 7th, 2020
 * DESCRIPTION: global configuration data for discovery
 */
#pragma once

#include <string>

namespace config
{
    extern std::string rom_name;
    extern std::string backup_path;
    extern std::string bios_name;
    extern bool show_help;
    extern bool debug; 
    
    // handle config file
    extern std::string config_file;
    void read_config_file(void);
};

