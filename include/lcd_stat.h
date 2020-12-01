/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: lcd_stat.h
 * DATE: August 27, 2020
 * DESCRIPTION: struct containing status information for the LCD
 */

#ifndef LCD_STAT_H
#define LCD_STAT_H

#include <queue>
#include "common.h"

struct lcd_stat
{
    u8 scanline;

    // REG_DISPCNT control
    struct dispcnt
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
    } dispcnt;

    // REG_DISPSTAT control
    struct dispstat
    {
        u8 in_vBlank : 1;
        u8 in_hBlank : 1;
        u8 vcs       : 1; // VCount trigger Status set if current scanline matches scanline trigger
        u8 vbi       : 1; // vblank irq
        u8 hbi       : 1; // hblank irq
        u8 vci       : 1; // vcount irq (fires interrupt when VCount trigger value == current scanline)
        u8 vct       : 8; // vcount trigger value
    } dispstat;

    // background controls (BGXCNT)
    struct bg_cnt
    {
        u8 priority    : 2; 
        u8 cbb         : 2; // character base block 
        u8 mosaic      : 1;
        u8 unused      : 2;
        u8 color_mode  : 1; // 16 colors (4bpp) if cleared; 256 colors (8bpp) if set
        u8 sbb         : 5; // screen base block
        u8 affine_wrap : 1;
        u8 size        : 2;
        u8 enabled     : 1;
    } bg_cnt[4]; // backgrounds 0-3

    // contains index of oam entry that must be updated
    std::queue<u8> oam_update;

    lcd_stat()
    {
        scanline = 0;

        // zero reg_dispcnt
        dispcnt.mode         = 0; 
        dispcnt.gb           = 0; 
        dispcnt.ps           = 0; 
        dispcnt.hb           = 0;
        dispcnt.obj_map_mode = 1; // default is 1d 
        dispcnt.fb           = 0; 
        dispcnt.bg_enabled   = 0; 
        dispcnt.obj_enabled  = 0; 
        dispcnt.win_enabled  = 0;

        // zero reg_dispstat
        dispstat.in_vBlank   = 0;
        dispstat.in_hBlank   = 0;
        dispstat.vcs         = 0;
        dispstat.vbi         = 0;
        dispstat.hbi         = 0;
        dispstat.vci         = 0;
        dispstat.vct         = 0;

        // zero background ctl
        for (int i = 0; i < 4; ++i)
        {
            bg_cnt[i].priority    = 0; 
            bg_cnt[i].cbb         = 0;  
            bg_cnt[i].mosaic      = 0; 
            bg_cnt[i].unused      = 0; 
            bg_cnt[i].color_mode  = 0; 
            bg_cnt[i].sbb         = 0; 
            bg_cnt[i].affine_wrap = 0; 
            bg_cnt[i].size        = 0; 
            bg_cnt[i].enabled     = 0;
        }
    }

    ~lcd_stat() { }
};

#endif // LCD_STAT_H