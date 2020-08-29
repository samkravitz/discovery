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
}

lcd_stat::~lcd_stat() { }