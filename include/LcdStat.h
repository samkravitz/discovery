/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: LcdStat.h
 * DATE: August 27, 2020
 * DESCRIPTION: struct containing status information for the LCD
 */

#pragma once

#include "common.h"

#include <cstring>

struct LcdStat
{
    u8 scanline;

    // REG_DISPCNT control
    struct DisplayControl
    {
        union 
        {
            struct
            {
                u8 mode         : 3; // video mode (0-5)
                u8 gb           : 1; // set if gbc game
                u8 ps           : 1; // page select
                u8 hb           : 1; // allows access to OAM in HBlank
                u8 obj_map_mode : 1; // 1D if set, 2d if cleared
                u8 fb           : 1; // forces a screen blank
                u8 bg_enabled   : 4; // bg0-bg3 enabled
                u8 obj_enabled  : 1; // set if sprites are enabled
                u8 win_enabled  : 3; // windows 0, 1, and object window enabled
            };

            u16 raw;
        };
        
    } dispcnt;

    // REG_DISPSTAT control
    struct DisplayStatus
    {
        union 
        {
            struct
            {
                u8 in_vBlank : 1;
                u8 in_hBlank : 1;
                u8 vcs       : 1; // VCount trigger Status: set if current scanline matches scanline trigger
                u8 vbi       : 1; // vblank irq
                u8 hbi       : 1; // hblank irq
                u8 vci       : 1; // vcount irq (fires interrupt when VCount trigger value == current scanline)
                u8 unused    : 2;
                u8 vct       : 8; // vcount trigger value
            };

            u16 raw;
        };

    } dispstat;


    // background controls (BGXCNT)
    struct BgControl
    {
        union
        {
            struct
            {
                u8 priority    : 2; 
                u8 cbb         : 2; // character base block
                u8 unused      : 2; 
                u8 mosaic      : 1;
                u8 color_mode  : 1; // 16 colors (4bpp) if cleared; 256 colors (8bpp) if set
                u8 sbb         : 5; // screen base block
                u8 affine_wrap : 1;
                u8 size        : 2;
            };

            u16 raw;
        };


        bool enabled;

        u32 dx;
        u32 dy;

        int width, height; // dimensions of map in pixels
        int voff,  hoff;   // vertical, horizontal offsets
    } bgcnt[4]; // backgrounds 0-3

    struct WinH
    {
        u8 right;
        u8 left;
    } winh[2];

    struct WinV
    {
        u8 bottom;
        u8 top;
    } winv[2];

    int window_content[4][6];

    void writeWinh(int win, u16 value)
    {
        u8 right = value >> 0 & 0xFF;
        u8 left  = value >> 8 & 0xFF;

        // Check for illegal values
        if (right > 240 || left > right)
            right = 240;
        
        winh[win].right = right;
        winh[win].left  = left;
    }

    void writeWinv(int win, u16 value)
    {
        u8 bottom = value >> 0 & 0xFF;
        u8 top    = value >> 8 & 0xFF;

        // Check for illegal values
        if (bottom > 160 || top > bottom)
            bottom = 160;
        
        winv[win].bottom = bottom;
        winv[win].top    = top;
    }

    void writeWindowContent(WindowContent win, u8 content)
    {
        for (int i = 0; i < 6; ++i)
            window_content[win][i] = content >> i & 1;
    }

    bool oam_changed;

    LcdStat()
    {
        scanline = 0;

        // zero reg_dispcnt
        dispcnt.raw = 0;

        // zero reg_dispstat
        dispstat.raw = 0;

        oam_changed = false;

        // zero background ctl
        for (int i = 0; i < 4; ++i)
        {
            bgcnt[i].raw     = 0;
            bgcnt[i].enabled = 0;
            bgcnt[i].dx      = 0;
            bgcnt[i].dy      = 0;
            bgcnt[i].width   = 0;
            bgcnt[i].height  = 0;
            bgcnt[i].voff    = 0;
            bgcnt[i].hoff    = 0;
        }

        // zero window control
        winh[0].left = winh[0].right = 0;
        winh[1].left = winh[1].right = 0;
        winv[0].bottom = winv[0].top = 0;
        winv[1].bottom = winv[1].top = 0;

        std::memset(window_content, 0, sizeof(window_content));
    }
};