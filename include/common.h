/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: common.h
 * DATE: July 17, 2020
 * DESCRIPTION: addresses of important mapped video and audio hardware and other common types in discovery
 */

#pragma once

#include <stdint.h>
#include <cstring>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// register mnemonics
constexpr u32 r0   = 0;
constexpr u32 r1   = 1;
constexpr u32 r2   = 2;
constexpr u32 r3   = 3;
constexpr u32 r4   = 4;
constexpr u32 r5   = 5;
constexpr u32 r6   = 6;
constexpr u32 r7   = 7;
constexpr u32 r8   = 8;
constexpr u32 r9   = 9;
constexpr u32 r10  = 10;
constexpr u32 r11  = 11;
constexpr u32 r12  = 12;
constexpr u32 r13  = 13;
constexpr u32 r14  = 14;
constexpr u32 r15  = 15;
constexpr u32 cpsr = 16;
constexpr u32 spsr = 17;

// IRQ bits
constexpr u32 IRQ_VBLANK  = 1 << 0;
constexpr u32 IRQ_HBLANK  = 1 << 1;
constexpr u32 IRQ_VCOUNT  = 1 << 2;
constexpr u32 IRQ_TIMER0  = 1 << 3;
constexpr u32 IRQ_TIMER1  = 1 << 4;
constexpr u32 IRQ_TIMER2  = 1 << 5;
constexpr u32 IRQ_TIMER3  = 1 << 6;
constexpr u32 IRQ_COM     = 1 << 7;
constexpr u32 IRQ_DMA0    = 1 << 8;
constexpr u32 IRQ_DMA1    = 1 << 9;
constexpr u32 IRQ_DMA2    = 1 << 10;
constexpr u32 IRQ_DMA3    = 1 << 11;
constexpr u32 IRQ_KEYPAD  = 1 << 12;
constexpr u32 IRQ_GAMEPAK = 1 << 13;

// LCD I/O Registers
constexpr u32 REG_DISPCNT  = 0x4000000;
constexpr u32 REG_DISPSTAT = 0x4000004;
constexpr u32 REG_VCOUNT   = 0x4000006;

constexpr u32 REG_BG0CNT   = 0x4000008;
constexpr u32 REG_BG1CNT   = 0x400000A;
constexpr u32 REG_BG2CNT   = 0x400000C;
constexpr u32 REG_BG3CNT   = 0x400000E;

constexpr u32 REG_BG0HOFS  = 0x4000010;
constexpr u32 REG_BG1HOFS  = 0x4000014;
constexpr u32 REG_BG2HOFS  = 0x4000018;
constexpr u32 REG_BG3HOFS  = 0x400001C;

constexpr u32 REG_BG0VOFS  = 0x4000012;
constexpr u32 REG_BG1VOFS  = 0x4000016;
constexpr u32 REG_BG2VOFS  = 0x400001A;
constexpr u32 REG_BG3VOFS  = 0x400001E;

// affine BG registers
constexpr u32 REG_BG2X     = 0x4000028;
constexpr u32 REG_BG2Y     = 0x400002C;
constexpr u32 REG_BG2PA    = 0x4000020;
constexpr u32 REG_BG2PB    = 0x4000022;
constexpr u32 REG_BG2PC    = 0x4000024;
constexpr u32 REG_BG2PD    = 0x4000026;

constexpr u32 REG_BG3X     = 0x4000038;
constexpr u32 REG_BG3Y     = 0x400003C;
constexpr u32 REG_BG3PA    = 0x4000030;
constexpr u32 REG_BG3PB    = 0x4000032;
constexpr u32 REG_BG3PC    = 0x4000034;
constexpr u32 REG_BG3PD    = 0x4000036;

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
