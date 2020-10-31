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
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

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

// affine BG registers
const u32 REG_BG2X_L   = 0x4000028;
const u32 REG_BG2X_H   = 0x400002A;
const u32 REG_BG2Y_L   = 0x400002C;
const u32 REG_BG2Y_H   = 0x400002E;
const u32 REG_BG2PA    = 0x4000020;
const u32 REG_BG2PB    = 0x4000022;
const u32 REG_BG2PC    = 0x4000024;
const u32 REG_BG2PD    = 0x4000026;

const u32 REG_BG3X_L   = 0x4000038;
const u32 REG_BG3X_H   = 0x400003A;
const u32 REG_BG3Y_L   = 0x400003C;
const u32 REG_BG3Y_H   = 0x400003E;
const u32 REG_BG3PA    = 0x4000030;
const u32 REG_BG3PB    = 0x4000032;
const u32 REG_BG3PC    = 0x4000034;
const u32 REG_BG3PD    = 0x4000036;

const u32 REG_MOSAIC   = 0x400004C;

// Sound registers

// DMA Transfer Channels
const u32 REG_DMA0SAD = 0x40000B0;
const u32 REG_DMA0DAD = 0x40000B4;
const u32 REG_DMA0CNT = 0x40000B8;
const u32 REG_DMA1SAD = 0x40000BC;
const u32 REG_DMA1DAD = 0x40000C0;
const u32 REG_DMA1CNT = 0x40000C4;
const u32 REG_DMA2SAD = 0x40000C8;
const u32 REG_DMA2DAD = 0x40000CC;
const u32 REG_DMA2CNT = 0x40000D0;
const u32 REG_DMA3SAD = 0x40000D4;
const u32 REG_DMA3DAD = 0x40000D8;
const u32 REG_DMA3CNT = 0x40000DC;


const u32 REG_DMA3CNT = 0x40000B8 + 36;

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
