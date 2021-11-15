/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * 
 * FILE: APU.cpp
 * DATE: Feb 13, 2021
 * DESCRIPTION: Implements the audio processing unit
 */

#include <cmath>
#include <cstring>
#include <iostream>
#include "APU.h"

constexpr int AMPLITUDE   = 5000;
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

    // queue up channel 3 if necessary
    if (stat->sndcnt3_l.enabled && (ticks % 100000 == 0))
    {
    }
}

void APU::bufferChannel1()
{
    if (stat->sndcnt1_x.reset == 0)
        return;

    SDL_LockAudioDevice(driver_id);
    auto &chan = channel[1];

    while (!chan.empty())
        chan.pop();
    
    int samples_buffered = 0;

    auto reg_freq_to_hz = [](int reg_freq) -> float
    {
        return 4194304 / (32 * (2048 - reg_freq));
    };

    auto reg_time_to_sec = [](int reg_time) -> float
    {
        return (64 - reg_time) * (1.0f / 256.0f);
    };

    int start_freq = stat->sndcnt1_x.freq;

    bool timed = stat->sndcnt1_x.timed;
    int reg_time = stat->sndcnt1_h.len;
    float max_time = reg_time_to_sec(reg_time);
    float time_elapsed = 0;

    float freq = reg_freq_to_hz(start_freq);
    int period = SAMPLE_RATE / freq;
    int hperiod = period / 2;
    int target_period = period;

    // get sweep data 
    int sweep_shifts = stat->sndcnt1_l.sweep_shifts;
    bool sweep_increase = !stat->sndcnt1_l.sweep_direction;
    int sweep_time_reg = stat->sndcnt1_l.sweep_time;

    bool sweep_enabled;
    float sweep_time;
    switch(sweep_time_reg)
    {
        case 0b000:
            // sweep is disabled
            sweep_enabled = 0;
            break;
        case 0b001:
        case 0b010:
        case 0b011:
        case 0b100:
        case 0b101:
        case 0b110:
        case 0b111:
            // sweep period is t/128*10^3 hz
            sweep_enabled = 1;
            sweep_time = sweep_time_reg / 128.0f;
            break;
        default:
            log(LogLevel::Error, "Invalid sweep time for sound channel 1!\n");
    }

    // get volume envelope data
    int env_step = stat->sndcnt1_h.env_step;
    bool env_enabled = env_step != 0;
    auto reg_step_to_sec = [](int reg_step) -> float
    {
        return reg_step * (1.0f / 64.0f);
    };
    float step_time = reg_step_to_sec(stat->sndcnt1_h.env_step);
    float time_since_last_env_step = 0, time_since_last_sweep_step = 0;
    int current_step = stat->sndcnt1_h.env_init;
    int remaining_sweeps = sweep_shifts;
    int volume;

    while (1)
    {
        // make sure we don't overflow the buffer
        // TODO - find out if there's an actual reason the buffer is being overflowed
        if (chan.size() >= 10000)
            break;

        samples_buffered++;
        
        if (current_step == 0)
            break;

        // number of samples wave will be low or high, per wave duty
        int lo, hi;
        switch (stat->sndcnt1_h.wave_duty)
        {
            case 0: lo = period * .875f; hi = period * .125f; break;
            case 1: lo = period * .75f;  hi = period * .25f;  break;
            case 2: lo = period * .5f;   hi = period * .5f;   break;
            case 3: lo = period * .25f;  hi = period * .75f;  break;
            default:
                log(LogLevel::Error, "Invalid wave duty for sound channel 1!\n");
        }
        
        // volume envelope
        if (env_enabled)
            volume = current_step / 15.0f * AMPLITUDE;
        else
            volume = AMPLITUDE;

        // push samples according to wave cycle
        for (int i = 0; i < lo; i++)
            chan.push(volume);
        for (int i = 0; i < hi; i++)
            chan.push(-volume);
       
        // sweep shift envelope 
        if (sweep_enabled && time_since_last_sweep_step >= sweep_time)
        {
            float factor = static_cast<float>(std::pow(2, sweep_shifts));
            
            if (factor >= 1)
            {
                float period_diff = period / factor;
                if (period_diff > 0.0f)
                    if (sweep_increase)
                        // frequency increases, so decrease period
                        period -= period_diff;
                    else
                        // frequency decreases, so increase period
                        period += period_diff;
            }
            if(remaining_sweeps > 0)
                remaining_sweeps--;
            time_since_last_sweep_step = 0;
        }

        // calculate time
        time_elapsed += static_cast<float>(period) / SAMPLE_RATE;
        time_since_last_env_step += static_cast<float>(period) / SAMPLE_RATE;
        time_since_last_sweep_step += static_cast<float>(period) / SAMPLE_RATE;

        // time has elapsed longer than the sound should be played for
        if (timed && time_elapsed >= max_time)
            break;

        if (env_enabled && time_since_last_env_step >= step_time)
        {
            if (stat->sndcnt1_h.env_mode)
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
    stat->sndcnt1_x.reset = 0;
    SDL_UnlockAudioDevice(driver_id);
}

void APU::bufferChannel2()
{
    if (stat->sndcnt2_h.reset == 0)
        return;

    SDL_LockAudioDevice(driver_id);
    auto &chan = channel[2];

    while (!chan.empty())
        chan.pop();
    
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
            break;

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
    stat->sndcnt2_h.reset = 0;
    SDL_UnlockAudioDevice(driver_id);
}

void APU::bufferChannel3()
{
    // TODO - channel 3
    return;
    SDL_LockAudioDevice(driver_id);
    auto &chan = channel[3];

    while (!chan.empty())
        chan.pop();
    
    // upper and lower 4-bit sample from wave ram
    u8 upper, lower;

    // wave ram is 1x64 bank
    if (stat->sndcnt3_l.bank_mode == 1)
    {
        for (int i = 0; i < 32; ++i)
        {
            upper = stat->wave_ram[i] >> 4;
            lower = stat->wave_ram[i] & 0xF;
            chan.push(upper / 15.0 * AMPLITUDE);
            chan.push(lower / 15.0 * AMPLITUDE);
        }
    }

    // wave ram is 2x32 banks
    else
    {
        // determine if using upper or lower bank
        u8 *wave_ram;
        if (stat->sndcnt3_l.bank_select == 0)
            wave_ram = &stat->wave_ram[0];
        else
            wave_ram = &stat->wave_ram[16];

        for (int i = 0; i < 16; ++i)
        {
            upper = wave_ram[i] >> 4;
            lower = wave_ram[i] & 0xF;
            chan.push(upper / 15.0 * AMPLITUDE);
            chan.push(lower / 15.0 * AMPLITUDE);
        }
    }
    
    //log("buffer 3\n");
    
    
    SDL_UnlockAudioDevice(driver_id);
}

