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

lcd_stat::lcd_stat() {
    current_scanline = 0;
    current_scanline_pixel = 0;
    in_vBlank = false;
    in_hBlank = false;
    needs_refresh = true;

    // zero reg_dispcnt
    reg_dispcnt.mode =
    reg_dispcnt.gb =
    reg_dispcnt.ps =
    reg_dispcnt.hb =
    reg_dispcnt.obj_enabled =
    reg_dispcnt.bg_enabled =
    reg_dispcnt.fb =
    reg_dispcnt.obj_map_mode =
    reg_dispcnt.win_enabled = 0;

    // zero background ctl
    for (int i = 0; i < 4; ++i)
        bg_cnt[i].priority = bg_cnt[i].cbb = bg_cnt[i].mosaic = bg_cnt[i].color_mode = bg_cnt[i].sbb = bg_cnt[i].affine_wrap = bg_cnt[i].size = bg_cnt[i].enabled = 0;
}

lcd_stat::~lcd_stat() { }