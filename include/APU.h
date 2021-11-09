/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Noah Bennett
 * 
 * FILE: APU.cpp
 * DATE: Feb 13, 2021
 * DESCRIPTION: Implements the audio processing unit
 */

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <iostream>
#include <queue>

#include "audio_stat.h"
#include "common.h"


class APU
{
public:
	APU(AudioStat *);
	~APU();

    std::queue<s16> channel[4];

    void bufferChannel(int);
    void tick();
	
private:
    AudioStat *stat;
	int driver_id; // SDL audio device driver
    u64 ticks;
};