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

#include "gpu.h"

struct lcd_stat {

    lcd_stat();
    ~lcd_stat();

    u32 lcd_clock;
    u32 current_scanline;

    GPU *gpu;

};

#endif // LCD_STAT_H