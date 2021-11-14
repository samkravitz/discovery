/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: audio_stat.h
 * DATE: November 8th, 2021
 * DESCRIPTION: struct containing status information for audio
 */

#pragma once

#include "common.h"

struct AudioStat
{
    struct SoundCntX
    {
        union
        {
            struct
            {
                u8 ch1_status    : 1;
                u8 ch2_status    : 1;
                u8 ch3_status    : 1;
                u8 ch4_status    : 1;
                u8 unused        : 2;
                u8 sound_enabled : 1;
                u8 unused2       : 8;
            };
            u16 raw;
        };
    } sndcnt_x;

    struct Sound2CntL
    {
        union
        {
            struct
            {
                u8 len       : 6;
                u8 wave_duty : 2;
                u8 env_step  : 3;
                u8 env_mode  : 1;
                u8 env_init  : 4;
            };
            u16 raw;
        };
    } sndcnt2_l;

    struct Sound2CntH
    {
        union
        {
            struct
            {
                u16 freq  : 11;
                u8 unused : 3;
                u8 timed  : 1; // set if timed, 0 if continuous
                u8 reset  : 1;
            };
            u16 raw;
        };
    } sndcnt2_h;

    struct Sound3CntL
    {
        union
        {
            struct
            {
                u8 unused      : 5;
                u8 bank_mode   : 1;
                u8 bank_select : 1;
                u8 enabled     : 1;
            };
            u16 raw;
        };
    } sndcnt3_l;

    struct Sound3CntH
    {
        union
        {
            struct
            {
                u8 len          : 8;
                u8 unused       : 5;
                u8 volume_ratio : 4; 
            };
            u16 raw;
        };
    } sndcnt3_h; 

    struct Sound3CntX
    {
        union
        {
            struct
            {
                u16 freq  : 11;
                u8 unused : 3;
                u8 timed  : 1; // set if timed, 0 if continuous
                u8 reset  : 1;
            };
            u16 raw;
        };
    } sndcnt3_x;

    AudioStat()
    {
        sndcnt_x.raw = 0;
        sndcnt2_l.raw = 0;
        sndcnt2_h.raw = 0;
        sndcnt3_l.raw = 0;
        sndcnt3_h.raw = 0;
        sndcnt3_x.raw = 0;
    }
};