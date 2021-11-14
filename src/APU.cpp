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

void APU::bufferChannel1() { }
void APU::bufferChannel2()
{
    auto &chan = channel[2];

    // "gracefully" empty channel
    std::queue<s16> empty;
    std::swap(chan, empty);
    
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

    float freq = reg_freq_to_hz(start_freq);
    int period = SAMPLE_RATE / freq;
    int hperiod = period / 2;

    // number of samples wave will be low or high, per wave duty
    int lo, hi;
    switch (stat->sndcnt2_l.wave_duty)
    {
        case 0: lo = period * .875f; hi = period * .125f; break;
        case 1: lo = period * .75f;  hi = period * .25f;  break;
        case 2: lo = period * .5f;   hi = period * .5f;   break;
        case 3: lo = period * .25f;  hi = period * .75f;  break;
        default:
            log(LogLevel::Error, "Invalid wave duty for sound channel 2!\n");
    }

    // get envelope data
    int env_step = stat->sndcnt2_l.env_step;
    bool env_enabled = env_step != 0;
    auto reg_step_to_sec = [](int reg_step) -> float
    {
        return reg_step * (1.0f / 64.0f);
    };
    float step_time = reg_step_to_sec(stat->sndcnt2_l.env_step);
    float time_since_last_env_step = 0;
    int current_step = stat->sndcnt2_l.env_init;

    int volume;

    while (1)
    {
        samples_buffered++;

        if (env_enabled)
            volume = current_step / 15.0f * AMPLITUDE;
        else
            volume = AMPLITUDE;
        
         if (current_step == 0)
            break;

        for (int i = 0; i < lo; i++)
            chan.push(volume);
        
        for (int i = 0; i < hi; i++)
            chan.push(-volume);
        
        time_elapsed += static_cast<float>(period) / SAMPLE_RATE;
        time_since_last_env_step += static_cast<float>(period) / SAMPLE_RATE;

        // time has elapsed longer than the sound should be played for
        if (timed && time_elapsed >= max_time)
            return;

        if (env_enabled && time_since_last_env_step >= step_time)
        {
            if (stat->sndcnt2_l.env_mode)
                current_step++;
            else
                current_step--;
            time_since_last_env_step = 0;
        }

        // arbitrary check to prevent infinite loop
        // TODO - calculate precisely how many samples this should be
        if (samples_buffered > 1000)
            break;        
    }

}
void APU::bufferChannel3() { }
void APU::bufferChannel4() { }