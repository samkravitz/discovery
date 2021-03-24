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

APU::APU(Memory *mem)
:mem(mem)
{
	SDL_Init(SDL_INIT_AUDIO);

	SDL_AudioSpec requested;
	requested.freq = 44100;
	requested.format = AUDIO_S16SYS;
	requested.channels = 2;
	requested.samples = 2048;
	requested.callback = sdlAudioCallback;
	requested.userdata = this;

	SDL_AudioSpec obtained;

	this->driver_id = SDL_OpenAudioDevice(nullptr, 0, &requested, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if(this->driver_id <= 0) std::cout << "SDL Error: " << SDL_GetError() << std::endl;	
	
	// sound output control
	if(1) {
		this->mem->Write16(REG_SOUNDCNT_L, 0x1177);
		u16 sound_cnt_l = (s16) this->mem->Read16(REG_SOUNDCNT_L);
		std::cout << "sound_cnt_l: " << sound_cnt_l << std::endl;
	}

	if(1) {
		this->mem->Write16(REG_SOUNDCNT_H, 0x2);
		u16 sound_cnt_h = (s16) this->mem->Read16(REG_SOUNDCNT_H);
		std::cout << "sound_cnt_h: " << sound_cnt_h << std::endl;
	}

	if(1) {
		this->mem->Write16(REG_SOUNDCNT_X, 0x80);
		u16 sound_cnt_x = (s16) this->mem->Read16(REG_SOUNDCNT_X);
		std::cout << "sound_cnt_x: " << sound_cnt_x << std::endl;
	}

	SDL_PauseAudioDevice(this->driver_id, 0);

	// u16 sound_cnt_h = (s16) this->mem->Read16(REG_SOUNDCNT_H);
	// u16 sound_cnt_x = (s16) this->mem->Read8(REG_SOUNDCNT_X);

	// std::cout << "sound_cnt_h: " << sound_cnt_h << std::endl;
	// std::cout << "sound_cnt_x: " << sound_cnt_x << std::endl;
	std::cout << "SDL_SOUNDISPLAYING: " << SDL_AUDIO_PLAYING << std::endl;
}

APU::~APU()
{
	SDL_CloseAudio();
}

bool APU::init() {

}

s8 APU::getDriverID() { 
	return this->driver_id; 
}

void APU::generateChannel1(s16 *stream, int buffer_len) 
{
	// dmg channel 1 sweep control
	u16 ch1_l = (s16) this->mem->Read8(REG_SOUND1CNT_L);
	u16 sweep_shifts = util::bitseq<2,0>(ch1_l);
	u16 sweep_asc_desc = util::bitseq<3,3>(ch1_l);
	u16 sweep_time = util::bitseq<6,4>(ch1_l);
	
	std::cout << "sweep_shift: " << sweep_shifts << std::endl;
	std::cout << "sweep_asc_desc: " << sweep_asc_desc << std::endl;
	std::cout << "sweep_time: " << sweep_time << std::endl;

	// dmg channel 1 wave and envelope control
	u16 ch1_h = (s16) this->mem->Read16(REG_SOUND1CNT_H);
	u16 sound_len = util::bitseq<5,0>(ch1_h);
	u16 wave_duty_cycle = util::bitseq<7,6>(ch1_h);
	u16 envelope_step_time = util::bitseq<0xA,8>(ch1_h);
	u16 envelope_mode = util::bitseq<0xB,0xB>(ch1_h);
	u16 envelope_init_value = util::bitseq<0xF,0xC>(ch1_h);

	std::cout << "sound_len: " << sound_len << std::endl;
	std::cout << "wave_duty_cycle: " << wave_duty_cycle << std::endl;
	std::cout << "envelope_step_time: " << envelope_step_time << std::endl;
	std::cout << "envelope_mode: " << envelope_mode << std::endl;
	std::cout << "envelope_init_value: " << envelope_init_value << std::endl;

	// dmg channel 1 frequency, reset, loop control
	u16 ch1_x = (s16) this->mem->Read16(REG_SOUND1CNT_X);
	u16 sound_freq = 4194304/(32*(2048-util::bitseq<0xA,0>(ch1_x)));
	bool timed_mode = (bool) util::bitseq<0xE,0xE>(ch1_x);
	bool sound_reset = (bool) util::bitseq<0xF,0xF>(ch1_x);
	
	std::cout << "sound_freq: " << sound_freq << std::endl;
	std::cout << "timed_mode: " << timed_mode << std::endl;
	std::cout << "sound_reset: " << sound_reset << std::endl;

	std::cout << "generate stream: " << stream << std::endl;


	for(int i = 0; i < buffer_len; i++) {
		// std::cout << "stream[" << i << "]: " << stream[i] << std::endl;
		
		for(int j = 0; j < buffer_len; j++) {
			// double time = (double) (stream / 44100);
			stream[i] = AMPLITUDE * std::sin(2.0 * M_PI * 441.0);
		}
	}
}

void sdlAudioCallback(void *_apu_ref, Uint8 *_stream_buffer, int _buffer_len) 
{
	s16 *stream = (s16*) _stream_buffer;
	int buffer_len = _buffer_len / 2;
	APU *apu = (APU*) _apu_ref;
	std::cout << "apu buff:" << apu << std::endl;
	std::cout << "stream buff:" << stream << std::endl;
	std::cout << "length of buff:" << buffer_len << std::endl;
	// std::vector<s16> ch1_stream(buffer_len);
	// std::vector<s16> ch2_stream(buffer_len);
	// std::vector<s16> ch3_stream(buffer_len);
	// std::vector<s16> ch4_stream(buffer_len);
	SDL_PauseAudioDevice(apu->getDriverID(), 0);


	// apu->generateChannel1(&ch1_stream[0], buffer_len);

	for(int i = 0; i < buffer_len; i++) {
		// s32 merged_stream_data = ch1_stream[i];
		// stream[i] = merged_stream_data;
		if(i % 2 == 0)
			stream[i] = 10000;
		else
			stream[i] = 60000;
		std::cout << "stream[" << i << "]: " << stream[i] << std::endl;
	}
}