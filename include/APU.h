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
#include "Scheduler.h"

// Direct Sound modes
constexpr int DS_MODE_DMA = 0;
constexpr int DS_MODE_INTERRUPT = 1;

struct APU_Channel_Output {
	// output
	std::vector<s16> stream;
	std::vector<s16> amplitude;

	u16 sound_frequency;

	bool use_left_output;
	bool use_right_output;
	bool is_playing;
};

struct APU_Direct_Sound_Output {
	u8 sample_rate_timer;
	bool fifo_reset;
	bool use_left_output;
	bool use_right_output;
	bool is_playing;
};

class APU {
	public:
	APU(Memory *, Scheduler *);
	~APU();

	Memory *mem;
	Scheduler *scheduler;

	// generate channel sound data
	void generateChannel1(void);
	void generateChannel2(void);
	// void generateChannel1(s16*, int, int);
	// void generateChannel2(s16*, int, int);
	void generateChannel3(s16*, int, int);
	void generateChannel4(s16*, int, int);
	void generateDirectSoundA(s16*, int, int);
	void generateDirectSoundB(s16*, int, int);

	// allocate channel memory with buffer length
	void allocateChannelMemory(void);

	// silence all output channels by clearing all four channel streams
	void clearChannelStreams(void);

	// wait for sound to play for n seconds
	void wait(double);

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

	// dmg output control
	u8 dmg_left_volume;
	bool vin_left_on;

	u8 dmg_right_volume;
	bool vin_right_on;

	u8 channels_output_ratio;
	u8 direct_sound_ratio_A;
	u8 direct_sound_ratio_B;

	// system sound config
	// amplitude -> ~volume
	int AMPLITUDE = 14000;
	
	// sample rate (frequency) -> number of sample frames sent to the computer's sound device per second
	int SAMPLE_RATE = 44100;
	
	// buffer size -> the size of the audio buffer in sample frames
	int BUFFER_SIZE = 4096;

	std::queue<APU_Channel_Output> output_queue;
	
	// device audio driver
	SDL_AudioDeviceID driver_id;
	
	int sample_size;
	u16 buffer_len;

	// apu enabled/disabled
	bool is_enabled;

	// sound channels 1 - 4
	struct APU_Channel_Output channel [4];

	// direct sound channels A and B
	struct APU_Direct_Sound_Output direct_sound [2];

};

/**
 * sdl audio callback function, calls audio 
 * callback function {SAMPLE_RATE} times / second
 * @param _apu_ref reference to the system apu
 * @param _stream_buffer reference to an array of data representing the stream data
 * @param _buffer_len size of the data stream
 */ 
void sdlAudioCallback(void*, Uint8*, int);

#include "APU.inl"
