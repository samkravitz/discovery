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

	bool Init();
	bool Close();
	void GenerateChannel1(s16*, int);
	void GenerateChannel2(s16*, int);
	void GenerateChannel3(s16*, int);
	void GenerateChannel4(s16*, int);
	void GenerateDirectSoundA(s16*, int);
	void GenerateDirectSoundB(s16*, int);
};

void AudioCallback(void*, Uint8*, int);
