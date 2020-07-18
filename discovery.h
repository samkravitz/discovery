/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: discovery.h
 * DATE: July 13, 2020
 * DESCRIPTION: Emulator class declaration
 */
#ifndef DISCOVERY_H
#define DISCOVERY_H

#include "cpu/arm_7tdmi.h"
#include "gpu/gpu.h"

class discovery {
    public:
        arm_7tdmi cpu;
        GPU gpu;
    
};

#endif // DISCOVERY_H