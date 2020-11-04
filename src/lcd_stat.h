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

#include "common/common.h"

struct lcd_stat
{
    u8 scanline;

    bool in_vBlank;
    bool in_hBlank;

    // REG_DISPCNT control
    struct reg_dispcnt
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
    } reg_dispcnt;

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

    lcd_stat()
    {
        scanline = 0;
        in_vBlank = false;
        in_hBlank = false;

        // zero reg_dispcnt
        reg_dispcnt.mode         = 0; 
        reg_dispcnt.gb           = 0; 
        reg_dispcnt.ps           = 0; 
        reg_dispcnt.hb           = 0;
        reg_dispcnt.obj_map_mode = 0; 
        reg_dispcnt.fb           = 0; 
        reg_dispcnt.bg_enabled   = 0; 
        reg_dispcnt.obj_enabled  = 0; 
        reg_dispcnt.win_enabled  = 0;

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