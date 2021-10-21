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
#include <vector>
#include <cmath>
#include <functional>
#include <vector>
#include <cassert>
#include <complex>
#include "APU.h"
#include "util.h"
#include "dmath.h"

// construct APU with discovery memory management unit
APU::APU(Memory *mem, Scheduler *scheduler):
mem(mem),
scheduler(scheduler),
audio_buffer(new CircularBuffer<s16>(this->NUM_SAMPLES))
{
	SDL_Init(SDL_INIT_AUDIO);
	std::cout<<"driver: "<<SDL_GetCurrentAudioDriver()<<std::endl;
	
	// define audio spec
	SDL_AudioSpec requested, obtained;
	requested.freq = this->SAMPLE_RATE;
	requested.format = AUDIO_S16SYS;
	requested.channels = this->NUM_CHANNELS;
	requested.samples = this->NUM_SAMPLES;
	requested.padding = 0;
	requested.callback = sdlAudioCallback;
	requested.userdata = this;
	// this->audio_buffer = new CircularBuffer<s16>(sizeof(s16[this->NUM_SAMPLES]));

	// this->setSampleSize(0);
	// this->setBufferLength(buffer_len);
	// this->allocateChannelMemory();
	// this->clearChannelStreams();

	this->mem->watcher->add(REG_SOUND1CNT_X, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND1CNT_X changed" << std::endl;
		this->generateChannel1();
		// SDL_Delay(10);
		// this->clearChannelStreams();
	});

	this->mem->watcher->add(REG_SOUND1CNT_L, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND1CNT_L changed" << std::endl;
		this->generateChannel1();
		// SDL_Delay(10);
		// this->clearChannelStreams();
	});

	this->mem->watcher->add(REG_SOUND1CNT_H, [this](u32 reg, u32 val) -> void {
		std::cout << "REG_SOUND1CNT_H changed" << std::endl;
		this->generateChannel1();
		// SDL_Delay(10);
		// this->clearChannelStreams();
	});

	// this->mem->watcher->add(REG_SOUND2CNT_L, [this](u32 reg, u32 val) -> void {
	// 	std::cout << "REG_SOUND2CNT_L changed" << std::endl;
	// 	this->generateChannel2();
	// 	// SDL_Delay(10);
	// 	this->clearChannelStreams();
	// });

	// this->mem->watcher->add(REG_SOUND2CNT_H, [this](u32 reg, u32 val) -> void {
	// 	std::cout << "REG_SOUND2CNT_H changed" << std::endl;
	//  this->generateChannel2();
	// 	// SDL_Delay(10);
	// 	this->clearChannelStreams();
	// });

	// select primary sound driver, nullptr here selects system default
	this->driver_id = SDL_OpenAudioDevice(nullptr, 0, &requested, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if(this->driver_id <= 0) std::cout << "SDL Error: " << SDL_GetError() << std::endl;	
	
	this->mem->watcher->add(REG_SOUNDCNT_L, [this](u32 reg, u32 val) -> void {
		// parse REG_SOUNDCNT_L
		// 2-0: DMG Left Volume
		// 3: Vin to Left on/off (?)
		// 6-4: DMG Right Volume
		// 7: Vin to Right on/off (?)
		// 8: DMG Sound 1 to left output
		// 9: DMG Sound 2 to left output
		// A: DMG Sound 3 to left output
		// B: DMG Sound 4 to left output
		// C: DMG Sound 1 to right output
		// D: DMG Sound 2 to right output
		// E: DMG Sound 3 to right output
		// F: DMG Sound 4 to right output 
		
		u16 sound_cnt_l = this->mem->read16(REG_SOUNDCNT_L);

		this->dmg_left_volume = util::bitseq<2,0>(sound_cnt_l);
		this->vin_left_on = (bool) util::bitseq<3,3>(sound_cnt_l);
		
		this->dmg_right_volume = (bool) util::bitseq<6,4>(sound_cnt_l);
		this->vin_right_on = (bool) util::bitseq<7,7>(sound_cnt_l);
		
		// left output
		this->channel[0].use_left_output = (bool) util::bitseq<8, 8>(sound_cnt_l);
		this->channel[1].use_left_output = (bool) util::bitseq<9, 9>(sound_cnt_l);
		this->channel[2].use_left_output = (bool) util::bitseq<0xA, 0xA>(sound_cnt_l);
		this->channel[3].use_left_output = (bool) util::bitseq<0xB, 0xB>(sound_cnt_l);

		// right output
		this->channel[0].use_right_output = (bool) util::bitseq<0xC, 0xC>(sound_cnt_l);
		this->channel[1].use_right_output = (bool) util::bitseq<0xD, 0xD>(sound_cnt_l);
		this->channel[2].use_right_output = (bool) util::bitseq<0xE, 0xE>(sound_cnt_l);
		this->channel[3].use_right_output = (bool) util::bitseq<0xF, 0xF>(sound_cnt_l);
	});

	this->mem->watcher->add(REG_SOUNDCNT_H, [this](u32 reg, u32 val) -> void {
		// 1-0: Output sound ratio for chan.1-4 (0=25%,1=50%,2=100%)
		// 2: Direct sound A output ratio (0=50%, 1=100%)
		// 3: Direct sound B output ratio (0=50%, 1=100%)
		// 7-4: Unused
		// 8: Direct sound A to right output
		// 9: Direct sound A to left output
		// A: Direct sound A Sampling rate timer (timer 0 or 1)
		// B: Direct sound A FIFO reset
		// C: Direct sound B to right output
		// D: Direct sound B to left output
		// E: Direct sound B Sampling rate timer (timer 0 or 1)
		// F: Direct sound B FIFO reset 
	
		u16 sound_cnt_h = this->mem->read16(REG_SOUNDCNT_H);

		// output ratios
		this->channels_output_ratio = util::bitseq<1, 0>(sound_cnt_h);
		this->direct_sound_ratio_A = util::bitseq<2, 2>(sound_cnt_h);
		this->direct_sound_ratio_B = util::bitseq<3, 3>(sound_cnt_h);

		// direct sound a
		this->direct_sound[0].use_right_output = util::bitseq<8, 8>(sound_cnt_h);
		this->direct_sound[0].use_left_output = util::bitseq<9, 9>(sound_cnt_h);
		this->direct_sound[0].sample_rate_timer = util::bitseq<0xA, 0xA>(sound_cnt_h);
		this->direct_sound[0].fifo_reset = util::bitseq<0xB, 0xB>(sound_cnt_h);

		// direct sound b
		this->direct_sound[1].use_right_output = util::bitseq<0xC, 0xC>(sound_cnt_h);
		this->direct_sound[1].use_left_output = util::bitseq<0xD, 0xD>(sound_cnt_h);
		this->direct_sound[1].sample_rate_timer = util::bitseq<0xE, 0xE>(sound_cnt_h);
		this->direct_sound[1].fifo_reset = util::bitseq<0xF, 0xF>(sound_cnt_h);
	});

	this->mem->watcher->add(REG_SOUNDCNT_X, [this](u32 reg, u32 val) -> void {
		// 0: DMG Sound 1 status
		// 1: DMG Sound 2 status
		// 2: DMG Sound 3 status
		// 3: DMG Sound 4 status
		// 6-4: Unused
		// 7: All Sound circuit enable (0=off, 1=on)
		// F-8: Unused 

		u16 sound_cnt_x = this->mem->read16(REG_SOUNDCNT_X);
		
		// dmg sound status
		this->channel[0].is_playing = util::bitseq<0, 0>(sound_cnt_x);
		this->channel[1].is_playing = util::bitseq<1, 1>(sound_cnt_x);
		this->channel[2].is_playing = util::bitseq<2, 2>(sound_cnt_x);
		this->channel[3].is_playing = util::bitseq<3, 3>(sound_cnt_x);
		
		// all sound circuits enabled
		this->is_enabled = util::bitseq<7, 7>(sound_cnt_x);
	});

	// enable SDL, calls audio callback function {SAMPLE_RATE} times / second
	SDL_PauseAudioDevice(this->driver_id, 0);
	std::cout << "SDL_SOUNDISPLAYING: " << SDL_AUDIO_PLAYING << std::endl;

}

APU::~APU() {
	// for(int i = 0; i < 4; i++) {
	// 	// delete[] this->channel[i].stream;
	// }
	SDL_CloseAudio();
}

// generate GBA channel 1 sounds, including square wave and frequency shifts
void APU::generateChannel1() {
	std::vector<s16> &x = this->channel[0].stream;
	std::vector<s16> &A = this->channel[0].amplitude;

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
	float D;
	u16 wave_duty_cycle_reg = util::bitseq<7,6>(ch1_h);
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
	int sample_length = this->BUFFER_LEN / 2;
	int P = this->BUFFER_LEN / 2;
	double time = 0;

	// generate sound
	// x[0] = f0;
	// A[0] = A0;
	int t = 0;
	std::vector<std::complex<double>> freqs;
	u32 samples = util::secondsToSamples(L);
	for(u16 i = 0; i < this->SAMPLE_RATE; i++) {
		// frequency function
		// std::function<double(int)> f = [R0, N, M](double t) -> double {
		// 	double Ri = R0 >> int( N * t );
		// 	double Rt = R0 + M ? Ri : ( -1 * Ri );
		// 	double ft = std::pow(2, 17) / ( 32 * ( 2048 - Rt ) );
		// 	return util::signum( std::sin(t * ft) );
		// };

		std::function<std::complex<double>(int)> f = [A0, this](double t) -> std::complex<double> {
			// std::cout<<std::sin(t)<<std::endl;
			return std::complex<double>(A0 * std::sin(440/this->SAMPLE_RATE * t), 0.);
		};

		// // amplitude function
		std::function<double(int)> A = [A0, Et, Me](double t) -> double {
			double Ai = std::pow(A0, int( -1 * t ));
			double At = A0 + Me ? Me : ( -1 * Ai );
			// if(Me) {
			// 	// Et becomes attack value
			// }
			return A0;
		};

		
		freqs.push_back(f(t));
		// s16 At = A(t);
		// // s16 At = A(t);
		// s16 xt = ft;


		// if(ft <= 0) break;
		// if(ft >= 131 * 1024) break;
		// if(At <= 0) break;
		// if(Mt && t > samples) break;
		// if(!this->mem->read16(REG_SOUND1CNT_L)) break;
		// if(!this->mem->read16(REG_SOUND1CNT_H)) break;

		// x.push_back(ft);
		t++;
		// this->sample_size += 1;
	}
	freqs = dmath::fourier(freqs);
	for(int i = 0; i < x.size();i++){
		x[i] = std::abs(freqs[i]);
		std::cout<<x[i]<< ',';
	}
	std::cout<<std::endl<<"endl"<<std::endl;

	// if(!Mt) {
	// 	// play sound for L (in milliseconds)
	// 	// otherwise sound is played continuously
	// 	this->wait(L);
	// }
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
	int sample_length = this->BUFFER_LEN / 2;
	double time = 0;

	// generate sound
	// x[0] = f0;
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

		// x[i] = A(i) * f(i);
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
	std::cout<<"waiting for ms: "<<ms<<std::endl;
	SDL_Delay(u32(ms));
}

void APU::allocateChannelMemory() {
	// resize all streams
	for(int i = 0; i < 4; i++) {
		this->channel[i].stream.resize(this->BUFFER_LEN);
		this->channel[i].amplitude.resize(this->NUM_SAMPLES);
	}
}

void sdlAudioCallback(void *_apu_ref, u8 *_stream_buffer, int _buffer_len) 
{
	APU *apu = (APU*) _apu_ref;
	s16 *stream = (s16*) _stream_buffer;
	u16 buffer_len = _buffer_len;
	u16 sample_count = 0;

	
	// initialize stream buffer to zero
	apu->setBufferLength(buffer_len);
	apu->allocateChannelMemory();

	// silence actual stream
	SDL_memset(stream, 0, buffer_len);

	// adjust the sample size per frame that needs to be passed to SDL
	if(buffer_len > apu->getBytesPerFrame()) {
		buffer_len = apu->getBytesPerFrame();
	}
	apu->setBufferLength(buffer_len);

	// todo: params: stream, source,  AUDIO_S16SYS, len, volume
	// SDL_MixAudioFormat(stream, merged_stream_data, AUDIO_S16SYS, )

	for(int i = 0; i < buffer_len/2; i+=1) {
		// s32 merged_stream_data = 0;
		// s16 ch1 = 0;
		
		// if(apu->getInternalBufferSize(0) > 0) {
		// 	ch1 = apu->getChannelStream(0, i);
		// 	merged_stream_data += ch1;
		// 	apu->popInternalBuffer(0);
		// }

		s16 ch1 = apu->getChannelStream(0, i);
		// s16 ch2 = apu->getChannelStream(1, i);
		// s16 ch3 = apu->getChannelStream(2, i);
		// s16 ch4 = apu->getChannelStream(3, i);

		
		s32 merged_stream_data = ch1;// + ch2 + ch3 + ch4;
		stream[i] = merged_stream_data;
		sample_count++;
	}
	std::cout<<"stream:"<<buffer_len<<std::endl;
	for(int i = 0; i < buffer_len/2; i++) {
		std::cout<<stream[i]<<",";
	}
	std::cout<<std::endl<<"end stream"<<std::endl;

}
