/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: config.h
 * DATE: December 7th, 2020
 * DESCRIPTION: global configuration data for discovery
 */
#pragma once

#include "common.h"

namespace Config
{
    std::string rom_name = "";
    std::string bios_name = "gba_bios.bin";

    bool debug = false;
}