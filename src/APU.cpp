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
	SDL_AudioSpec requested;
	requested.freq = 44100;
	requested.format = AUDIO_S16SYS;
	requested.channels = 1;
	requested.samples = 2048;
	requested.callback = AudioCallback;
	requested.userdata = this;

	SDL_AudioSpec obtained;
	SDL_OpenAudio(&requested, &obtained);
	SDL_PauseAudio(0);
}

APU::~APU()
{
	SDL_CloseAudio();
}

void APU::GenerateChannel1(s16 *stream, int buffer_len) 
{
	u16 ch1_l = (s16) this->mem->Read8(REG_SOUND1CNT_L);
	
	u16 sweep_shifts = util::bitseq<2,0>(ch1_l);
	u16 sweep_asc_desc = util::bitseq<3,3>(ch1_l);
	u16 sweep_time = util::bitseq<6,4>(ch1_l);
	
	std::cout << "ch1_l: " << ch1_l << std::endl;
	std::cout << "sweep_shift: " << sweep_shifts << std::endl;
	std::cout << "sweep_asc_desc: " << sweep_asc_desc << std::endl;
	std::cout << "sweep_time: " << sweep_time << std::endl;
}

void AudioCallback(void *_apu_ref, unsigned char *_stream, int _buffer_len) 
{
	s16 *stream = (s16*) _stream;
	int buffer_len = _buffer_len / 2;
	APU *apu = (APU*) _apu_ref;
	
	std::vector<s16> ch1_stream(buffer_len);
	std::vector<s16> ch2_stream(buffer_len);
	std::vector<s16> ch3_stream(buffer_len);
	std::vector<s16> ch4_stream(buffer_len);

	apu->GenerateChannel1(&ch1_stream[0], buffer_len);
}