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

APU::APU(AudioStat *stat) :
    stat(stat)
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
    }
}

void APU::bufferChannel(int ch)
{
    if (ch < 0 || ch >= 4)
    {
        log(LogLevel::Error, "Invalid channel given to APU::bufferChannel!\n");
        return;
    }

    auto &chan = channel[ch];

    // "gracefully" empty channel
    std::queue<s16> empty;
    std::swap(chan, empty);

    int volume = AMPLITUDE;
    
    int samples_buffered = 0;

    auto reg_freq_to_hz = [](int reg_freq) -> float
    {
        return 4194304 / (32 * (2048 - reg_freq));
    };

    auto reg_time_to_sec = [](int reg_time) -> float
    {
        return (64 - reg_time) * (1.0f / 256.0f);
    };

    int start_freq = stat->sndcnt2_h.freq;

    bool timed = stat->sndcnt2_h.timed;
    int reg_time = stat->sndcnt2_l.len;
    float max_time = reg_time_to_sec(reg_time);
    float time_elapsed = 0;

    while (1)
    {
        samples_buffered++;
        float freq = reg_freq_to_hz(start_freq);
        int period = SAMPLE_RATE / freq;
        int hperiod = period / 2;

        if (volume == 0)
            break;

        for (int j = 0; j < hperiod; j++)
            chan.push(volume);
        
        for (int j = 0; j < hperiod; j++)
            chan.push(-volume);
        
        time_elapsed += static_cast<float>(period) / SAMPLE_RATE;
        // time has elapsed longer than the sound should be played for
        if (timed && time_elapsed >= max_time)
            return;
        
        if (samples_buffered % 100 == 0)
            volume /= 2;
    }

}