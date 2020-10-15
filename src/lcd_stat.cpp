/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: lcd_stat.h
 * DATE: August 27, 2020
 * DESCRIPTION: struct containing status information for the LCD
 */

#include "lcd_stat.h"

lcd_stat::lcd_stat()
{
    current_scanline = 0;
    current_scanline_pixel = 0;
    in_vBlank = false;
    in_hBlank = false;
    needs_refresh = true;

    // zero reg_dispcnt
    reg_dispcnt = {0};

    // zero background ctl
    for (int i = 0; i < 4; ++i)
        bg_cnt[i] = {0};
}

lcd_stat::~lcd_stat() { }