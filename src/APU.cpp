/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * 
 * FILE: APU.cpp
 * DATE: Feb 13, 2021
 * DESCRIPTION: Implements the audio processing unit
 */

#include <iostream>
#include <queue>
#include <cmath>
#include <vector>

#include "APU.h"
#include "util.h"

const int AMPLITUDE = 28000;
const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 2048;

APU::APU(Memory *mem)
:mem(mem)
{
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec requested;
	requested.freq = SAMPLE_RATE;
	requested.format = AUDIO_S16SYS;
	requested.channels = 2;
	requested.samples = BUFFER_SIZE;
	requested.callback = sdlAudioCallback;
	requested.userdata = this;

	SDL_AudioSpec obtained;

	this->driver_id = SDL_OpenAudioDevice(nullptr, 0, &requested, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if(this->driver_id <= 0) std::cout << "SDL Error: " << SDL_GetError() << std::endl;	
	
	// sound output control
	this->mem->write16(REG_SOUNDCNT_L, 0x1177);
	u16 sound_cnt_l = (s16) this->mem->read16(REG_SOUNDCNT_L);
	std::cout << "sound_cnt_l: " << sound_cnt_l << std::endl;

	this->mem->write16(REG_SOUNDCNT_H, 0x2);
	u16 sound_cnt_h = (s16) this->mem->read16(REG_SOUNDCNT_H);
	std::cout << "sound_cnt_h: " << sound_cnt_h << std::endl;

	this->mem->write16(REG_SOUNDCNT_X, 0x80);
	u16 sound_cnt_x = (s16) this->mem->read16(REG_SOUNDCNT_X);
	std::cout << "sound_cnt_x: " << sound_cnt_x << std::endl;

	SDL_PauseAudioDevice(this->driver_id, 0);
	std::cout << "SDL_SOUNDISPLAYING: " << SDL_AUDIO_PLAYING << std::endl;
}

APU::~APU()
{
	SDL_CloseAudio();
}

bool APU::init() {

}

s8 APU::getDriverID() 
{ 
	return this->driver_id; 
}

void APU::generateChannel1(s16 *stream, int buffer_len, int sample_count) 
{
	// prototypes
	double calcSweepShifts(double, int);

	// dmg channel 1 sweep control
	// sweep shifts unit (s)
	// sweep asc == 0, desc == 1
	u16 ch1_l = (s16) this->mem->read8(REG_SOUND1CNT_L);
	u16 sweep_shift_reg = util::bitseq<2,0>(ch1_l);
	u16 sweep_asc_desc = util::bitseq<3,3>(ch1_l);
	u16 sweep_time_reg = util::bitseq<6,4>(ch1_l);
	
	// dmg channel 1 wave and envelope control
	u16 ch1_h = (s16) this->mem->read16(REG_SOUND1CNT_H);
	u16 sound_len_reg = util::bitseq<5,0>(ch1_h);
	u16 sound_len = (64 - sound_len_reg)/256;
	u16 wave_duty_cycle = util::bitseq<7,6>(ch1_h);
	u16 envelope_step_time = util::bitseq<0xA,8>(ch1_h);
	u16 envelope_mode = util::bitseq<0xB,0xB>(ch1_h);
	u16 envelope_init_value = util::bitseq<0xF,0xC>(ch1_h);

	// dmg channel 1 frequency, reset, loop control
	u16 ch1_x = (s16) this->mem->read16(REG_SOUND1CNT_X);
	u16 sound_freq_reg = util::bitseq<0xA,0>(ch1_x);
	u16 sound_freq = 4194304 / (32 * (2048 - sound_freq_reg));
	bool timed_mode = (bool) util::bitseq<0xE,0xE>(ch1_x);
	bool sound_reset = (bool) util::bitseq<0xF,0xF>(ch1_x);
	
	// sample length, AUDIO_S16SYS is 2 bits
	int sample_len = buffer_len/2;

	// generate sound
	for(int i = 0; i < sample_len; i++) {
		double period = (double) sample_count / (double) SAMPLE_RATE;
		double wave = (s16) AMPLITUDE * std::sin(2.0 * M_PI * sound_freq * period);
		double sweep_shift = period + sweep_asc_desc
			? (period / std::pow(2, sweep_shift_reg))
			: -1 * (period / std::pow(2, sweep_shift_reg));
		double sweep_time = sweep_time_reg / 128000;
		stream[i] = wave + sweep_shift;
		sample_count += 1;
	}
}

void sdlAudioCallback(void *_apu_ref, Uint8 *_stream_buffer, int _buffer_len) 
{
	APU *apu = (APU*) _apu_ref;
	s16 *stream = (s16*) _stream_buffer;
	int buffer_len = _buffer_len/2;
	int sample_count = 0;

	s16 *ch1_stream = new s16[buffer_len];
	s16 *ch2_stream = new s16[buffer_len];
	s16 *ch3_stream = new s16[buffer_len];
	s16 *ch4_stream = new s16[buffer_len];

	// initialize audio buffer with silence
	for(int i = 0; i < buffer_len; i++) {
		ch1_stream[i] = 0;
		ch2_stream[i] = 0;
		ch3_stream[i] = 0;
		ch4_stream[i] = 0;
	}

	SDL_PauseAudioDevice(apu->getDriverID(), 0);

	apu->generateChannel1(ch1_stream, buffer_len, sample_count);
	// apu->generateChannel2(&ch2_stream, buffer_len, sample_count);
	// apu->generateChannel3(&ch3_stream, buffer_len, sample_count);
	// apu->generateChannel4(&ch4_stream, buffer_len, sample_count);

	for(int i = 0; i < buffer_len; i++) {
		s32 merged_stream_data = ch1_stream[i] + ch2_stream[i] + ch3_stream[i] + ch4_stream[i];
		stream[i] = merged_stream_data;
	}
}