/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: common.h
 * DATE: July 17, 2020
 * DESCRIPTION: addresses of important mapped video and audio hardware and other common types in discovery
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

const u32 REG_DISPCNT  = 0x4000000;
const u32 REG_KEYINPUT = 0x4000130;
const u32 REG_IE       = 0x4000200; // interrupt enable register
const u32 REG_IF       = 0x4000202; // interrupt request flags
const u32 REG_IME      = 0x4000208; // master interrupt enable

#endif // COMMON_H
