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
};

#endif // LCD_STAT_H