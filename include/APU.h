/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * 
 * FILE: APU.cpp
 * DATE: Feb 13, 2021
 * DESCRIPTION: Implements the audio processing unit
 */

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include "Memory.h"

// Direct Sound modes
constexpr int DS_MODE_DMA = 0;
constexpr int DS_MODE_INTERRUPT = 1;

class APU
{
	public:
	APU(Memory *mem);
	~APU();

	Memory *mem;

	void GenerateChannel1();
	void GenerateChannel2();
	void GenerateChannel3();
	void GenerateChannel4();
	void GenerateDirectSoundA();
	void GenerateDirectSoundB();
};

void AudioCallback(void*, unsigned char*, int);