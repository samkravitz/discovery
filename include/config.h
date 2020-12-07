/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: config.h
 * DATE: December 7th, 2020
 * DESCRIPTION: Global configuration data for discovery
 */
#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

namespace util
{
    std::string rom_name = "";
    std::string bios_name = "gba_bios.bin";

    bool debug = false;
}

#endif // CONFIG_H