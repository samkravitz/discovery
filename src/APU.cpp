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
#include <functional>
#include <vector>
#include <cassert>
#include "APU.h"
#include "util.h"

// construct APU with discovery memory management unit
APU::APU(Memory *mem, Scheduler *scheduler)
:mem(mem)
,scheduler(scheduler)
{
	SDL_Init(SDL_INIT_AUDIO);
	std::cout<<"drive: "<<SDL_GetCurrentAudioDriver()<<std::endl;
	
	// define audio spec
	SDL_AudioSpec requested, obtained;
	requested.freq = this->SAMPLE_RATE;
	requested.format = AUDIO_S16SYS;
	requested.channels = 1;
	requested.samples = this->BUFFER_SIZE;
	requested.callback = sdlAudioCallback;
	requested.userdata = this;

	this->setSampleSize(0);
	this->setBufferLength(buffer_len);
	this->allocateChannelMemory();
	this->clearChannelStreams();

	// int ndevises = SDL_GetNumAudioDevices(1);
	// std::cout<<ndevises<<std::endl;
	// for(int i = 0; i < ndevises; i++) {
	// 	std::cout<<SDL_GetAudioDeviceName(i,1)<<std::endl;
	// }

	this->mem->watcher->add(REG_SOUND1CNT_X, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND1CNT_X changed" << std::endl;
		this->generateChannel1();
		SDL_Delay(10);
		this->clearChannelStreams();
	});

	this->mem->watcher->add(REG_SOUND1CNT_L, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND1CNT_L changed" << std::endl;
		this->generateChannel1();
		SDL_Delay(10);
		this->clearChannelStreams();
	});

	this->mem->watcher->add(REG_SOUND1CNT_H, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND1CNT_H changed" << std::endl;
		this->generateChannel1();
		SDL_Delay(10);
		this->clearChannelStreams();
	});

	this->mem->watcher->add(REG_SOUND2CNT_L, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND2CNT_L changed" << std::endl;
		this->generateChannel2();
		SDL_Delay(10);
		this->clearChannelStreams();
	});

	this->mem->watcher->add(REG_SOUND2CNT_H, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND2CNT_H changed" << std::endl;
		this->generateChannel2();
		SDL_Delay(10);
		this->clearChannelStreams();
	});

	// select primary sound driver, nullptr here selects system default
	this->driver_id = SDL_OpenAudioDevice(nullptr, 0, &requested, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if(this->driver_id <= 0) std::cout << "SDL Error: " << SDL_GetError() << std::endl;	
	
	// sound output control
	this->mem->write16(REG_SOUNDCNT_L, 0x1177);
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
	std::vector<s16> &x = this->channel[0].stream;

	// dmg channel 1 sweep control
	// sweep shifts unit (s)
	// sweep_freq_direction => freq asc == 0, desc == 1
	u16 ch1_l = (s16) this->mem->read8(REG_SOUND1CNT_L);
	
	// N -> sweep shift number (not rate, but a factor applied to rate)
	u16 N = util::bitseq<2, 0>(ch1_l);

	// M -> sweep direction mode (inc, dec)
	u16 M = util::bitseq<3, 3>(ch1_l);
	
	// T -> sweep step time in seconds
	u16 T = util::bitseq<6, 4>(ch1_l);
	
	// dmg channel 1 wave and envelope control
	// envelope_mode => volume asc == 1, desc == 0
	// init envelope value => 1111 max vol, 0000 silence
	u16 ch1_h = (s16) this->mem->read16(REG_SOUND1CNT_H);

	// L -> length of sound being played in seconds
	u16 sound_len_reg = util::bitseq<5, 0>(ch1_h);
	double L = ( 64 - (double) sound_len_reg ) / 256.;
	// std::cout<<"L: "<<L<<std::endl;

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

	// used to calculate envelope step time
	u16 envelope_step_time_reg = util::bitseq<0xA,8>(ch1_h);
	
	// Et -> envelope step time in seconds, functions as attack/decay rate
	u16 Et = envelope_step_time_reg / 64;

	// Me -> envelope direction mode, default to descending (inc, desc)
	// when 0 wave volume decays at rate Et, when 1 wave has attack of Et
	u16 Me = util::bitseq<0xB, 0xB>(ch1_h);
	
	// A0 -> initial ampliude, envelope initial value
	double A0 = ( util::bitseq<0xF, 0xC>(ch1_h) / (double) 0b1111 ) * AMPLITUDE;

	// dmg channel 1 frequency, reset, loop control
	u16 ch1_x = (s16) this->mem->read16(REG_SOUND1CNT_X);
	
	// R0 -> initial sound rate
	u16 R0 = util::bitseq<0xA, 0>(ch1_x);

	// R -> current sound rate, used to find current frequency
	u16 R = R;
	
	// f0 -> initial sound wave frequency 
	u16 f0 = std::pow(2, 17) / ( 32 * ( 2048 - R ) );
	
	// Mt -> Timed mode, if 0, sound plays forever, if 1, plays until decays to 0
	bool Mt = util::bitseq<0xE, 0xE>(ch1_x);
	
	// Re -> sound reset, resets sound to initial volume
	bool Re = util::bitseq<0xF, 0xF>(ch1_x);
	
	// sample length, AUDIO_S16SYS is 2 bits
	int sample_length = buffer_len / 2;
	double time = 0;

	// generate sound
	x[0] = f0;
	for(int i = 1; i < sample_length; i++) {
		// frequency function
		std::function<double(int)> f = [R0, N, M](double t) -> double {
			double Ri = R0 >> int( N * t );
			double Rt = R0 + M ? Ri : ( -1 * Ri );
			double ft = std::pow(2, 17) / ( 32 * ( 2048 - Rt ) );
			return util::signum( std::sin(t * ft) );
		};
		
		// amplitude function
		std::function<double(int)> A = [A0, Et, Me](double t) -> double {
			double Ai = std::pow(A0, int( -1 * t ));
			double At = A0 + Me ? Me : ( -1 * Ai );
			// if(Me) {
			// 	// Et becomes attack value
			// }
			return A0;
		};
		// std::cout<<"A(t): "<<A(i)<<", f(t): "<<f(i)<<std::endl;
		x[i] = A(i) * f(i);
		this->sample_size += 1;
	}

	if(!Mt) {
		// play sound for L (in milliseconds)
		// otherwise sound is played continuously
		this->wait(L);
	}
	std::cout<<"generated channel 1"<<std::endl;
}

void APU::generateChannel2() {
	std::vector<s16> &x = this->channel[1].stream;

	// dmg channel 1 sweep control
	// sweep shifts unit (s)
	// sweep_freq_direction => freq asc == 0, desc == 1
	u16 ch2_l = (s16) this->mem->read8(REG_SOUND2CNT_L);

	u16 sound_len_reg = util::bitseq<5, 0>(ch2_l);
	double L = ( 64 - (double) sound_len_reg ) / 256.;

	// D -> wave duty cycle, ratio between on/off time
	u16 wave_duty_cycle_reg = util::bitseq<7,6>(ch2_l);
	float D;
	switch(wave_duty_cycle_reg) {
		// calculate quadrangular wave ratio
		case 0b00: D = .125; break;
		case 0b01: D = .250; break;
		case 0b10: D = .500; break;
		case 0b11: D = .750; break;
		default: assert(!"Invalid wave cycle ratio register value");
	}

	// used to calculate envelope step time
	u16 envelope_step_time_reg = util::bitseq<0xA,8>(ch2_l);
	
	// Et -> envelope step time in seconds, functions as attack/decay rate
	u16 Et = envelope_step_time_reg / 64;

	// Me -> envelope direction mode, default to descending (inc, desc)
	// when 0 wave volume decays at rate Et, when 1 wave has attack of Et
	u16 Me = util::bitseq<0xB, 0xB>(ch2_l);
	
	// A0 -> initial ampliude, envelope initial value
	double A0 = ( util::bitseq<0xF, 0xC>(ch2_l) / (double) 0b1111 ) * AMPLITUDE;

	// init envelope value => 1111 max vol, 0000 silence
	u16 ch2_h = (s16) this->mem->read16(REG_SOUND2CNT_H);

	u16 R0 = util::bitseq<0xA, 0>(ch2_h);
	
	// R -> current sound rate, used to find current frequency
	u16 R = R;
	
	// f0 -> initial sound wave frequency 
	u16 f0 = std::pow(2, 17) / ( 32 * ( 2048 - R ) );

	// Mt -> Timed mode, if 0, sound plays forever, if 1, plays until decays to 0
	bool Mt = util::bitseq<0xE, 0xE>(ch2_h);
	
	// Re -> sound reset, resets sound to initial volume
	bool Re = util::bitseq<0xF, 0xF>(ch2_h);
	
	// sample length, AUDIO_S16SYS is 2 bits
	int sample_length = buffer_len / 2;
	double time = 0;

	// generate sound
	x[0] = f0;
	for(int i = 1; i < sample_length; i++) {
		// frequency function
		std::function<double(int)> f = [R0](double t) -> double {
			double ft = std::pow(2, 17) / ( 32 * ( 2048 - R0 ) );
			return util::signum( std::sin(t * ft) );
		};
		
		// amplitude function
		std::function<double(int)> A = [A0, Et, Me](double t) -> double {
			double Ai = std::pow(A0, int( -1 * t ));
			double At = A0 + Me ? Me : ( -1 * Ai );
			// if(Me) {
			// 	// Et becomes attack value
			// }
			return A0;
		};

		x[i] = A(i) * f(i);
		this->sample_size += 1;
	}

	if(!Mt) {
		// play sound for L (in milliseconds)
		// otherwise sound is played continuously
		this->wait(L);
	}
	std::cout<<"generated channel 2"<<std::endl;
}

void APU::wait(double ms) {
	// std::cout<<"waiting for ms: "<<ms<<std::endl;

	// SDL_Delay(u32(ms));
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

	// initialize stream buffer to zero
	// apu->setSampleSize(0);
	apu->setBufferLength(buffer_len);
	apu->allocateChannelMemory();
	// apu->clearChannelStreams();

	if(buffer_len % 20 == 0)
	std::cout<<"sdlCallback"<<std::endl;

	// silence actual stream
	for(int i = 0; i < buffer_len; i++) {
		stream[i] = 0;
	}

	SDL_PauseAudioDevice(apu->getDriverID(), 0);

	// apu->generateChannel1();
	// apu->generateChannel2();
	// apu->generateChannel1(ch1_stream, buffer_len, sample_count);
	// apu->generateChannel2(ch2_stream, buffer_len, sample_count);
	// apu->generateChannel3(&ch3_stream, buffer_len, sample_count);
	// apu->generateChannel4(&ch4_stream, buffer_len, sample_count);

	for(int i = 0; i < buffer_len; i+=1) {
		s16 ch1 = apu->getChannelStream(0, i);
		s16 ch2 = apu->getChannelStream(1, i);
		s16 ch3 = apu->getChannelStream(2, i);
		s16 ch4 = apu->getChannelStream(3, i);
		s32 merged_stream_data = ch1 + ch2 + ch3 + ch4;
		stream[i] = merged_stream_data;
		sample_count++;
	}
}
