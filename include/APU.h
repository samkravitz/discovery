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

class APU {
	public:
	APU(Memory *mem);
	~APU();

	Memory *mem;

	// generate channel sound data
	void generateChannel1(s16*, int, int);
	void generateChannel2(s16*, int, int);
	void generateChannel3(s16*, int, int);
	void generateChannel4(s16*, int, int);
	void generateDirectSoundA(s16*, int, int);
	void generateDirectSoundB(s16*, int, int);

	// system sound getters & setters
	inline int getAmplitude(void);
	inline void setAmplitude(int);

	inline int getSampleRate(void);
	inline void setSampleRate(int);

	inline int getBufferSize(void);
	inline void setBufferSize(int);

	inline s8 getDriverID(void);
	inline void setDriverID(s8);
	
	private:
	// system sound config
	int AMPLITUDE = 14000;
	int SAMPLE_RATE = 441000;
	int BUFFER_SIZE = 4096;
	
	// device audio driver
	SDL_AudioDeviceID driver_id;

	// sound channels 1 - 4
	struct output_channel {
		u16 sound_frequency;
		double output_ampliude;

	} channel [4];
};

void sdlAudioCallback(void*, Uint8*, int);

#include "APU.inl"
