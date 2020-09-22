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

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;

// LCD I/O Registers
const u32 REG_DISPCNT  = 0x4000000;
const u32 REG_DISPSTAT = 0x4000004;
const u32 REG_VCOUNT   = 0x4000006;

const u32 REG_BG0CNT   = 0x4000008;
const u32 REG_BG1CNT   = 0x400000A;
const u32 REG_BG2CNT   = 0x400000C;
const u32 REG_BG3CNT   = 0x400000E;

const u32 REG_BG0HOFS  = 0x4000010;
const u32 REG_BG1HOFS  = 0x4000014;
const u32 REG_BG2HOFS  = 0x4000018;
const u32 REG_BG3HOFS  = 0x400001C;

const u32 REG_BG0VOFS  = 0x4000012;
const u32 REG_BG1VOFS  = 0x4000016;
const u32 REG_BG2VOFS  = 0x400001A;
const u32 REG_BG3VOFS  = 0x400001E;


// Sound registers

// DMA Transfer Channels

// Timer Registers

// Keypad Input
const u32 REG_KEYINPUT = 0x4000130;

// Serial Communication

// Interrupt, Waitstate, and Power down control
const u32 REG_IE       = 0x4000200; // interrupt enable register
const u32 REG_IF       = 0x4000202; // interrupt request flags
const u32 WAITCNT      = 0x4000204; // waitstate control
const u32 REG_IME      = 0x4000208; // master interrupt enable

#endif // COMMON_H
