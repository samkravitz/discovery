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
#include <vector>
#include <queue>
#include "Memory.h"

// Direct Sound modes
constexpr int DS_MODE_DMA = 0;
constexpr int DS_MODE_INTERRUPT = 1;

struct APU_Output {
	double freq;
	double ampl;
	int samplesLeft;
};

class APU {
	public:
	APU(Memory *mem);
	~APU();

	Memory *mem;

	// generate channel sound data
	void generateChannel1(void);
	// void generateChannel1(s16*, int, int);
	void generateChannel2(s16*, int, int);
	void generateChannel3(s16*, int, int);
	void generateChannel4(s16*, int, int);
	void generateDirectSoundA(s16*, int, int);
	void generateDirectSoundB(s16*, int, int);

	// allocate channel memory with buffer length
	void allocateChannelMemory(void);

	// silence all output channels by clearing all four channel streams
	void clearChannelStreams(void);

	// inline system sound getters & setters
	inline s16 getChannelStream(int, int);
	inline void setChannelStream(int, u16, s16*);

	inline int getAmplitude(void);
	inline void setAmplitude(int);

	inline int getSampleRate(void);
	inline void setSampleRate(int);

	inline int getBufferSize(void);
	inline void setBufferSize(int);

	inline int getSampleSize(void);
	inline void setSampleSize(int);

	inline u16 getBufferLength(void);
	inline void setBufferLength(u16);

	inline s8 getDriverID(void);
	inline void setDriverID(s8);
	
	private:
	// system sound config
	int AMPLITUDE = 14000;
	int SAMPLE_RATE = 44100;
	int BUFFER_SIZE = 4096;

	std::queue<APU_Output> output_queue;
	
	// device audio driver
	SDL_AudioDeviceID driver_id;
	
	int sample_size;
	u16 buffer_len;

	// sound channels 1 - 4
	struct output_channel {
		std::vector<s16> stream;
		u16 sound_frequency;
		double output_ampliude;

		bool is_playing;

	} channel [4];
};

/**
 * sdl audio callback function
 * @param _apu_ref reference to the system apu
 * @param _stream_buffer reference to an array of data representing the stream data
 * @param _buffer_len size of the data stream
 */ 
void sdlAudioCallback(void*, Uint8*, int);

#include "APU.inl"
