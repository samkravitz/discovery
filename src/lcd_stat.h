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
    lcd_stat();
    ~lcd_stat();

    u32 current_scanline;
    u16 current_scanline_pixel;

    bool in_vBlank;
    bool in_hBlank;

    bool needs_refresh;

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

};

#endif // LCD_STAT_H