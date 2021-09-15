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
#include <cassert>
#include "APU.h"
#include "util.h"

// construct APU with discovery memory management unit
APU::APU(Memory *mem)
:mem(mem)
{
	SDL_Init(SDL_INIT_AUDIO);
	std::cout<<"drive: "<<SDL_GetCurrentAudioDriver()<<std::endl;
	
	// define audio spec
	SDL_AudioSpec requested, obtained;
	requested.freq = this->SAMPLE_RATE;
	requested.format = AUDIO_S16SYS;
	requested.channels = 2;
	requested.samples = this->BUFFER_SIZE;
	requested.callback = sdlAudioCallback;
	requested.userdata = this;

	int ndevises = SDL_GetNumAudioDevices(1);
	std::cout<<ndevises<<std::endl;
	for(int i = 0; i < ndevises; i++) {
		std::cout<<SDL_GetAudioDeviceName(i,1)<<std::endl;
	}

	// select primary sound driver, nullptr here selects system default
	this->driver_id = SDL_OpenAudioDevice(nullptr, 0, &requested, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if(this->driver_id <= 0) std::cout << "SDL Error: " << SDL_GetError() << std::endl;	
	
	// sound output control
	this->mem->write16(REG_SOUNDCNT_L, 0x0B0F);
	u16 sound_cnt_l = (s16) this->mem->read16(REG_SOUNDCNT_L);
	std::cout << "sound_cnt_l: " << sound_cnt_l << std::endl;

	this->mem->write16(REG_SOUNDCNT_H, 0x2);
	u16 sound_cnt_h = (s16) this->mem->read16(REG_SOUNDCNT_H);
	std::cout << "sound_cnt_h: " << sound_cnt_h << std::endl;

	// turn sound on
	this->mem->write16(REG_SOUNDCNT_X, 0x80);
	u16 sound_cnt_x = (s16) this->mem->read16(REG_SOUNDCNT_X);
	std::cout << "sound_cnt_x: " << sound_cnt_x << std::endl;

	SDL_PauseAudioDevice(this->driver_id, 0);
	std::cout << "SDL_SOUNDISPLAYING: " << SDL_AUDIO_PLAYING << std::endl;

}

APU::~APU() {
	for(int i = 0; i < 4; i++) {
		// delete[] this->channel[i].stream;
	}
	SDL_CloseAudio();
}

// generate GBA channel 1 sounds, including square wave and frequency shifts
void APU::generateChannel1() {
	std::vector<s16> &stream = this->channel[0].stream;

	// dmg channel 1 sweep control
	// sweep shifts unit (s)
	// sweep_freq_direction => freq asc == 0, desc == 1
	u16 ch1_l = (s16) this->mem->read8(REG_SOUND1CNT_L);
	
	// N -> sweep shift number (not rate, but a factor applied to rate)
	u16 N = util::bitseq<2,0>(ch1_l);

	// M -> sweep direction mode (inc, dec)
	u16 M = util::bitseq<3,3>(ch1_l);
	
	// T -> sweep step time
	u16 T = util::bitseq<6,4>(ch1_l);
	
	// dmg channel 1 wave and envelope control
	// envelope_mode => volume asc == 1, desc == 0
	// init envelope value => 1111 max vol, 0000 silence
	u16 ch1_h = (s16) this->mem->read16(REG_SOUND1CNT_H);

	// L -> length of sound being played in seconds
	u16 sound_len_reg = util::bitseq<5,0>(ch1_h);
	u16 L = ( 64 - sound_len_reg ) / 256;

	// D -> wave duty cycle, ratio between on/off time
	u16 wave_duty_cycle_reg = util::bitseq<7,6>(ch1_h);
	float D;
	switch(wave_duty_cycle_reg) {
		// calculate quadrangular wave ratio
		case 0b00: D = .125; break;
		case 0b01: D = .250; break;
		case 0b10: D = .500; break;
		case 0b11: D = .750; break;
		default: assert(!"Invalid wave cycle ratio register value");
	}

	// Et -> envelope step time
	u16 envelope_step_time_reg = util::bitseq<0xA,8>(ch1_h);
	u16 Et = envelope_step_time_reg / 64;

	// Me -> envelope direction mode (inc, dec)
	u16 Me = util::bitseq<0xB,0xB>(ch1_h);
	
	// V0 -> initial volume, envelope initial value
	u16 V0 = util::bitseq<0xF,0xC>(ch1_h);

	// dmg channel 1 frequency, reset, loop control
	u16 ch1_x = (s16) this->mem->read16(REG_SOUND1CNT_X);
	
	// F -> sound frequency 
	u16 sound_freq_reg = util::bitseq<0xA,0>(ch1_x);
	u16 F = std::pow(2, 17) / ( 2048 - sound_freq_reg );
	
	// Mt -> Timed mode, if 0, sound plays forever, if 1, plays until decays to 0
	bool Mt = util::bitseq<0xE,0xE>(ch1_x);
	
	// Re -> sound reset, resets sound to initial volume
	bool Re = util::bitseq<0xF,0xF>(ch1_x);
	
	// L -> sample length, AUDIO_S16SYS is 2 bits
	int L = buffer_len / 2;
	double time = 0;

	// generate sound
	for(int i = 1; i < sample_len; i++) {
		
		stream[i] = ;
		this->sample_size += 1;
	}
	SDL_Delay(sound_len);
}

void APU::generateChannel2(s16 *stream, int buffer_len, int sample_count) {
	// dmg channel 2, wave duty and envelope control
	u16 ch2_l = (s16) this->mem->read8(REG_SOUND2CNT_L);
	u16 sound_len_reg = util::bitseq<5,0>(ch2_l);
	u16 sound_len = (64 - sound_len_reg) / 256;
	u16 wave_duty_cycle_reg = util::bitseq<7,6>(ch2_l);
	u16 env_step_time = util::bitseq<0xA, 8>(ch2_l);
	bool env_mode = util::bitseq<0xB, 0xB>(ch2_l);
	u16 init_env_value = util::bitseq<0xF, 0xC>(ch2_l);

	float wave_cycle_ratio;
	switch(wave_duty_cycle_reg) {
		case 0b00: wave_cycle_ratio = .125; break;
		case 0b01: wave_cycle_ratio = .250; break;
		case 0b10: wave_cycle_ratio = .500; break;
		case 0b11: wave_cycle_ratio = .750; break;
		default: assert(!"Invalid wave_cycle_ratio register value");
	}

	// dmg channel 2, frequency, reset, loop control
	u16 ch2_h = (s16) this->mem->read8(REG_SOUND2CNT_H);
	u16 sound_freq_reg = util::bitseq<0xA, 0>(ch2_h);
	u16 sound_freq = 4194304 / (32 * (2048 - sound_freq_reg));
	bool timed_mode = util::bitseq<0xE, 0xE>(ch2_h);
	bool sound_reset = util::bitseq<0xF, 0xF>(ch2_h);

	// sample length, AUDIO_S16SYS is 2 bits
	int sample_len = buffer_len/2;

	// generate sound
	for(int i = 1; i < sample_len; i++) {
		double time = (double) sample_count / (double) SAMPLE_RATE;
		double wave = (s16) AMPLITUDE * std::sin(2.0 * M_PI * sound_freq * time);
		stream[i] = wave;
		sample_count += 1;
	}
}

void APU::allocateChannelMemory() {
	// resize all streams
	for(int i = 0; i < 4; i++) {
		this->channel[i].stream.resize(this->buffer_len);
	}
}

void APU::clearChannelStreams() {
	// zero all four streams
	for(int i = 0; i < 4; i++) {
		std::fill(this->channel[i].stream.begin(), this->channel[i].stream.end(), 0);
	}
}

void sdlAudioCallback(void *_apu_ref, Uint8 *_stream_buffer, int _buffer_len) 
{
	APU *apu = (APU*) _apu_ref;
	s16 *stream = (s16*) _stream_buffer;
	int buffer_len = _buffer_len/2;
	int sample_count = 0;

	apu->setSampleSize(0);
	apu->setBufferLength(buffer_len);
	apu->allocateChannelMemory();
	apu->clearChannelStreams();

	SDL_PauseAudioDevice(apu->getDriverID(), 0);

	apu->generateChannel1();
	// apu->generateChannel1(ch1_stream, buffer_len, sample_count);
	// apu->generateChannel2(ch2_stream, buffer_len, sample_count);
	// apu->generateChannel3(&ch3_stream, buffer_len, sample_count);
	// apu->generateChannel4(&ch4_stream, buffer_len, sample_count);

	for(int i = 0; i < buffer_len; i++) {
		s16 ch1 = apu->getChannelStream(0, i);
		s16 ch2 = apu->getChannelStream(1, i);
		s16 ch3 = apu->getChannelStream(2, i);
		s16 ch4 = apu->getChannelStream(3, i);
		s32 merged_stream_data = ch1 + ch2 + ch3 + ch4;
		stream[i] = merged_stream_data;
	}
	
}
