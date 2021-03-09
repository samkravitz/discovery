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
	std::cout << "initialised APU" << std::endl;
}

APU::~APU()
{
	SDL_CloseAudio();
}

void APU::GenerateChannel1(s16 *stream, int buffer_len) 
{
	float ch1_l = (s16) this->mem->Read8(REG_SOUND1CNT_L);
	std::cout << "ch1_l: " << ch1_l << std::endl;
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