/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: common.h
 * DATE: July 17, 2020
 * DESCRIPTION: addresses of important mapped video and audio hardware and other common types in discovery
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <ctime>
#include <SDL2/SDL.h>

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

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

// cpu mode
enum class Mode
{
    USR = 0b10000, // The normal ARM program execution state
    FIQ = 0b10001, // Designed to support a data transfer or channel process
    IRQ = 0b10010, // Used for general-purpose interrupt handling
    SVC = 0b10011, // Protected mode for the operating system
    ABT = 0b10111, // Entered after a data or instruction prefetch abort
    SYS = 0b11111, // A privileged user mode for the operating system
    UND = 0b11011  // Entered when an undefined instruction is executed
};

// cpu state 
enum class State
{
    ARM,
    THUMB
};

// condition code flag of program status register
enum class ConditionFlag
{
    N, // 31st bit 
    Z, // 30th bit
    C, // 29th bit
    V  // 28th bit
};

// condition field of an instruction
// first 4 bits of an instruction
enum class Condition
{
    EQ = 0b0000, // Z set,                       equal
    NE = 0b0001, // Z clear,                     not equal
    CS = 0b0010, // C set,                       unsigned >=
    CC = 0b0011, // C clear,                     unsigned <
    MI = 0b0100, // N set,                       negative
    PL = 0b0101, // N clear,                     positive or 0
    VS = 0b0110, // V set,                       overflow
    VC = 0b0111, // V clear,                     no overflow
    HI = 0b1000, // C set and Z clear,           unsigned >
    LS = 0b1001, // C clear or Z set,            unsigned <=
    GE = 0b1010, // N equals V,                  >=
    LT = 0b1011, // N not equal to V,            <
    GT = 0b1100, // Z clear AND (N equals V),    >
    LE = 0b1101, // Z set OR (N not equal to V), <=
    AL = 0b1110  // (ignored),                   always
    // 0b1111 is a noop
};

/* see docs/arm_instruction_set_bitfield.png to see a visual of the different types of instructions,
 * here are the abbreviations that I'm using:
 * 
 * DP - Data Processing
 * PSR - PSR Transfer
 * MUL - Multiply 
 * MULL - Multiply long
 * SDS - Single Data Swap
 * BEX - Branch and Exchange
 * HDT - Halfword Data Transfer
 * SDT - Single Data Transfer
 * UNDEF - Undefined
 * BDT - Block Data Transfer
 * B - Branch
 * CDT -  Coprocessor Data Transfer
 * CDO - Coprocessor Data Operation
 * CRT - Coprocessor Register Transfer
 * INT - Software Interrupt
 */
enum class ArmInstruction
{
    DP,
    PSR,
    MUL,
    MULL, 
    SWP,
    BEX,
    HDT,
    SDT,
    UNDEF,
    BDT,
    B,
    CDT,
    CDO,
    CRT,
    INT,
};

typedef enum DataProcessingOpcodes
{
    AND = 0b0000, // op1 AND op2
    EOR = 0b0001, // op1 XOR op2
    SUB = 0b0010, // op1 - op2
    RSB = 0b0011, // op2 - op1
    ADD = 0b0100, // op1 + op2
    ADC = 0b0101, // op1 + op2 + carry
    SBC = 0b0110, // op1 - op2 + carry - 1
    RSC = 0b0111, // op2 - op1 + carry - 1
    TST = 0b1000, // as AND, but result is not written
    TEQ = 0b1001, // as EOR, but result is not written
    CMP = 0b1010, // as SUB, but result is not written
    CMN = 0b1011, // as ADD, but result is not written
    ORR = 0b1100, // op1 OR op2
    MOV = 0b1101, // op2 (op1 is ignored)
    BIC = 0b1110, // op1 AND NOT op2 (bit clear)
    MVN = 0b1111 // NOT op2 (op1 is ignored)
} DataProcessingOpcodes;

enum class ThumbInstruction
{
    MSR,    // move shifted register
    ADDSUB, // add/subtract
    IMM,    // move/compare/add/subtract immediate
    ALU,    // ALU operations
    HI,     // Hi register operations
    PC,     // PC relative load
    MOV,    // load/store with register offset
    MOVS,   // load/store sign extended byte/halfword
    MOVI,   // load/store with immediate offset
    MOVH,   // load/store halfword
    SP,     // SP relative load/store
    LDA,    // load address
    ADDSP,  // add offset to stack pointer
    POP,    // push/pop registers
    MOVM,   // multiple load/store
    B,      // conditional branch
    SWI,    // software interrupt
    BAL,    // unconditional branch
    BL,     // long branch with link
    UND   // undefined
};

enum class InterruptOccasion
{
    VBLANK,
    HBLANK,
    VCOUNT,
    TIMER0,
    TIMER1,
    TIMER2,
    TIMER3,
    COM,
    DMA0,
    DMA1,
    DMA2,
    DMA3,
    KEYPAD,
    GAMEPAK,
};

enum class BackupType
{
    SRAM,
    EEPROM,
    FLASH,
    NONE
};

enum WindowContent
{
    CONTENT_WIN0   = 0,
    CONTENT_WIN1   = 1,
    CONTENT_WINOUT = 2,
    CONTENT_WINOBJ = 3
};

