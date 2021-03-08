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

    LcdStat()
    {
        scanline = 0;

        // zero reg_dispcnt
        dispcnt.raw = 0;

        // zero reg_dispstat
        dispstat.raw = 0;

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
    }

    ~LcdStat() { }
};