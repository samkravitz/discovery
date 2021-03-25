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
#include <iostream>

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

	bool init(void);
	bool close(void);
	void generateChannel1(s16*, int, int);
	void generateChannel2(s16*, int, int);
	void generateChannel3(s16*, int, int);
	void generateChannel4(s16*, int, int);
	void generateDirectSoundA(s16*, int, int);
	void generateDirectSoundB(s16*, int, int);

	s8 getDriverID(void);
	
	private:
	// device audio driver
	SDL_AudioDeviceID driver_id;
};

void sdlAudioCallback(void*, Uint8*, int);
