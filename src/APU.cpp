/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * 
 * FILE: APU.cpp
 * DATE: Feb 13, 2021
 * DESCRIPTION: Implements the audio processing unit
 */

#include "APU.h"
#include <iostream>

APU::APU(Memory *mem):mem(mem)
{

}

APU::~APU()
{
	
}

void APU::GenerateChannel1() 
{
	float ch1_l = (s16) this->mem->Read8(REG_SOUND1CNT_L);
	std::cout << "ch1_l: " << ch1_l << std::endl;
}