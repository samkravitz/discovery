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

APU::APU(Memory *mem)
:mem(mem)
{
	SDL_Init(SDL_INIT_AUDIO);
	for(int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
		const char *driver_name = SDL_GetAudioDriver(i);
		std::cout << "driver: " << driver_name << std::endl;
	}

	SDL_AudioSpec requested;
	requested.freq = 44100;
	requested.format = AUDIO_S16SYS;
	requested.channels = 1;
	requested.samples = 2048;
	requested.callback = AudioCallback;
	requested.userdata = this;

	SDL_AudioSpec obtained;

	int driver_id;
	if((driver_id = SDL_OpenAudioDevice(nullptr, 0, &requested, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE)) <= 0) {
		std::cout << "SDL Error: " << SDL_GetError() << std::endl;
	}
	std::cout << "selected driver: " << driver_id << std::endl;

	SDL_PauseAudioDevice(driver_id, 0);
	// for(int freq = 440; freq < 880; freq += 10)
	// 	SDL_Delay(10);
	// for(int freq = 870; freq > 450; freq -= 10)
	// 	SDL_Delay(10);
}

APU::~APU()
{
	SDL_CloseAudio();
}

bool APU::Init() {

}

void APU::GenerateChannel1(s16 *stream, int buffer_len) 
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

	bool is_playing = false;
	
	if(timed_mode) {
		is_playing = false;
	} else {
		is_playing = true;
	}

	if(is_playing) {
		std::cout << "audio should be playing i think" << std::endl;
		SDL_PauseAudio(0);
		SDL_Delay(2000);
	}
}



void AudioCallback(void *_apu_ref, Uint8 *_stream_buffer, int _buffer_len) 
{
	s16 *stream = (s16*) _stream_buffer;
	int buffer_len = _buffer_len / 2;
	APU *apu = (APU*) _apu_ref;
	std::cout << "stream buff:" << _stream_buffer << std::endl;
	std::vector<s16> ch1_stream(buffer_len);
	std::vector<s16> ch2_stream(buffer_len);
	std::vector<s16> ch3_stream(buffer_len);
	std::vector<s16> ch4_stream(buffer_len);

	apu->GenerateChannel1(&ch1_stream[0], buffer_len);
}