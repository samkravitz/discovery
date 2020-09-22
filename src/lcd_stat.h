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

struct lcd_stat {
    lcd_stat();
    ~lcd_stat();

    u32 current_scanline;
    u16 current_scanline_pixel;

    bool in_vBlank;
    bool in_hBlank;

    bool needs_refresh;

    // REG_DISPCNT control
    u8 mode = 0;
    bool obj;

    // background controls (BGXCNT)
    struct bg_cnt {
        u8 priority    : 2; 
        u8 cbb         : 2; // character base block 
        u8 mosaic      : 1;
        u8 color_mode  : 1; // 16 colors (4bpp) if cleared; 256 colors (8bpp) if set
        u8 sbb         : 5; // screen base block
        u8 affine_wrap : 1;
        u8 size        : 2;
    } bg_cnt[4]; // backgrounds 0-3

};

#endif // LCD_STAT_H