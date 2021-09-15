/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: config.h
 * DATE: December 7th, 2020
 * DESCRIPTION: global configuration data for discovery
 */
#include "config.h" 

namespace config
{
    std::string rom_name = "";
    std::string backup_path = "";
    std::string bios_name = "gba_bios.bin";
    bool show_help = false;
    bool debug = false;
    double framerate = 0.;
}