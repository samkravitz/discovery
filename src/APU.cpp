/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * 
 * FILE: APU.cpp
 * DATE: Feb 13, 2021
 * DESCRIPTION: Implements the audio processing unit
 */
#include "APU.h"
#include "util.h"

#include <iostream>
#include <queue>
#include <cmath>
#include <vector>
#include <cassert>

constexpr int AMPLITUDE   = 14000;
constexpr int SAMPLE_RATE = 48000;
constexpr int BUFFER_SIZE = 2048;

/**
 * SDL audio callback
 * @param userdata pointer to instance of APU
 * @param stream pointer to the audio data buffer to be filled
 * @param len length of that buffer in bytes
 */
void callback(void *userdata, u8 *stream, int len)
{
	s16 *snd = reinterpret_cast<s16*>(stream);
	for(int i = 0; i < len / 2; i++)
	{
		snd[i] = 32000 * sin(t);
		
		t += 440 * M_PI * 2 / 48000.0;
		if(t >= M_PI * 2)
			t -= M_PI * 2;
	}
}

APU::APU()
{
	SDL_Init(SDL_INIT_AUDIO);

    ticks = 0;
	
	// define audio spec
	SDL_AudioSpec spec, aspec;
	spec.freq = SAMPLE_RATE;
	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.samples = BUFFER_SIZE;
	spec.callback = callback;
	spec.userdata = this;

	// select primary sound driver, nullptr here selects system default
	driver_id = SDL_OpenAudioDevice(nullptr, 0, &spec, &aspec, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if (driver_id <= 0)
        std::cout << "SDL Error: " << SDL_GetError() << std::endl;	
	
	SDL_PauseAudioDevice(driver_id, 0);
	std::cout << "SDL_SOUNDISPLAYING: " << SDL_AUDIO_PLAYING << std::endl;
}

APU::~APU()
{
	SDL_CloseAudio();
}

void APU::tick()
{
    ++ticks;

    // one frame
    if (ticks % 280896 == 0)
    {
    }
}