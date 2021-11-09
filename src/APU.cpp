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

#include <cmath>
#include <cstring>
#include <iostream>

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
    APU *apu = reinterpret_cast<APU*>(userdata);
	s16 *snd = reinterpret_cast<s16*>(stream);

    // zero stream
    std::memset(stream, 0, len);

    s16 merged_stream_data;
	for(int i = 0; i < len / 2; i++)
	{
        merged_stream_data = 0;
        for (int c = 0; c < 4; c++)
        {
            auto &channel = apu->channel[c];
            if (!channel.empty())
            {
                merged_stream_data += channel.front();
                channel.pop();
            }
        }
        snd[i] = merged_stream_data;
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

    static bool is_low = true;

    // queue up one frame's worth of audio
    if (ticks % 280896 == 0)
    {
        constexpr int desired_freq = 440;
        int period = SAMPLE_RATE / desired_freq;
        int hperiod = period / 2;
        for (int i = 0; i < SAMPLE_RATE; i++)
        {
            s16 val = is_low ? 10000 : -10000;
            channel[2].push(val);
            if (i % hperiod == 0)
                is_low = !is_low;
        }
    }
}