/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: mmio.h
 * DATE: January 6, 2021
 * DESCRIPTION: memory mapped IO register addresses
 */
#pragma once

#include "common.h"

// LCD I/O Registers
constexpr u32 REG_DISPCNT  = 0x4000000;
constexpr u32 REG_DISPSTAT = 0x4000004;
constexpr u32 REG_VCOUNT   = 0x4000006;

constexpr u32 REG_BG0CNT   = 0x4000008;
constexpr u32 REG_BG1CNT   = 0x400000A;
constexpr u32 REG_BG2CNT   = 0x400000C;
constexpr u32 REG_BG3CNT   = 0x400000E;

constexpr u32 REG_BG0HOFS  = 0x4000010;
constexpr u32 REG_BG0VOFS  = 0x4000012;
constexpr u32 REG_BG1HOFS  = 0x4000014;
constexpr u32 REG_BG1VOFS  = 0x4000016;
constexpr u32 REG_BG2HOFS  = 0x4000018;
constexpr u32 REG_BG2VOFS  = 0x400001A;
constexpr u32 REG_BG3HOFS  = 0x400001C;
constexpr u32 REG_BG3VOFS  = 0x400001E;

// affine BG registers
constexpr u32 REG_BG2PA    = 0x4000020;
constexpr u32 REG_BG2PB    = 0x4000022;
constexpr u32 REG_BG2PC    = 0x4000024;
constexpr u32 REG_BG2PD    = 0x4000026;
constexpr u32 REG_BG2X     = 0x4000028;
constexpr u32 REG_BG2Y     = 0x400002C;

constexpr u32 REG_BG3PA    = 0x4000030;
constexpr u32 REG_BG3PB    = 0x4000032;
constexpr u32 REG_BG3PC    = 0x4000034;
constexpr u32 REG_BG3PD    = 0x4000036;
constexpr u32 REG_BG3X     = 0x4000038;
constexpr u32 REG_BG3Y     = 0x400003C;

// window registers
constexpr u32 REG_WIN0H    = 0x4000040;
constexpr u32 REG_WIN1H    = 0x4000042;
constexpr u32 REG_WIN0V    = 0x4000044;
constexpr u32 REG_WIN1V    = 0x4000046;
constexpr u32 REG_WININ    = 0x4000048;
constexpr u32 REG_WINOUT   = 0x400004A;

constexpr u32 REG_MOSAIC   = 0x400004C;

// Sound registers

// DMA Transfer Channels
constexpr u32 REG_DMA0SAD  = 0x40000B0;
constexpr u32 REG_DMA0DAD  = 0x40000B4;
constexpr u32 REG_DMA0CNT  = 0x40000B8;
constexpr u32 REG_DMA1SAD  = 0x40000BC;
constexpr u32 REG_DMA1DAD  = 0x40000C0;
constexpr u32 REG_DMA1CNT  = 0x40000C4;
constexpr u32 REG_DMA2SAD  = 0x40000C8;
constexpr u32 REG_DMA2DAD  = 0x40000CC;
constexpr u32 REG_DMA2CNT  = 0x40000D0;
constexpr u32 REG_DMA3SAD  = 0x40000D4;
constexpr u32 REG_DMA3DAD  = 0x40000D8;
constexpr u32 REG_DMA3CNT  = 0x40000DC;

// Timer Registers
constexpr u32 REG_TM0D     = 0x4000100;
constexpr u32 REG_TM0CNT   = 0x4000102;
constexpr u32 REG_TM1D     = 0x4000104;
constexpr u32 REG_TM1CNT   = 0x4000106;
constexpr u32 REG_TM2D     = 0x4000108;
constexpr u32 REG_TM2CNT   = 0x400010A;
constexpr u32 REG_TM3D     = 0x400010C;
constexpr u32 REG_TM3CNT   = 0x400010E;

// Keypad Input
constexpr u32 REG_KEYINPUT = 0x4000130;
constexpr u32 REG_KEYCNT   = 0x4000132;

// Serial Communication

// Interrupt, Waitstate, and Power down control
constexpr u32 REG_IE       = 0x4000200; // interrupt enable register
constexpr u32 REG_IF       = 0x4000202; // interrupt request flags
constexpr u32 WAITCNT      = 0x4000204; // waitstate control
constexpr u32 REG_IME      = 0x4000208; // master interrupt enable

// GBA System control
constexpr u32 REG_HALTCNT  = 0x4000301;