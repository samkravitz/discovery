/* discovery
 * License: MIT
 * See LICENSE.txt for full license text
 * 
 * FILE: common.h
 * DATE: June 19, 2020
 * DESCRIPTION: common typedefs throughout arm7tdmi use
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t halfword;
typedef uint32_t word;
typedef uint32_t arm_instruction;

// cpu state
typedef enum State {
    ARM,
    THUMB
} state_t;

// cpu mode 
typedef enum Mode {
    USR, // The normal ARM program execution state
    FIQ, // Designed to support a data transfer or channel process
    IRQ, // Used for general-purpose interrupt handling
    SVC, // Protected mode for the operating system
    ABT, // Entered after a data or instruction prefetch abort
    SYS, // A privileged user mode for the operating system
    UND // Entered when an undefined instruction is executed
} cpu_mode_t;

// condition code flag of program status register
typedef enum ConditionCodeFlag {
    N, // 31st bit 
    Z, // 30th bit
    C, // 29th bit
    V // 28th bit
} condition_code_flag_t;

// condition field of an instruction
// first 4 bits of an instruction
typedef enum Condition {
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
} condition_t;

/* see docs/arm_instruction_set_bitfield.png to see a visual of the different types of instructions,
 * here are the abbreviations that I'm using:
 * 
 * DP - Data Processing/PSR Transfer
 * MUL - Multiply 
 * MULL - Multiply Long
 * SDS - Single Data Swap
 * BEX - Branch and Exchange
 * HDT_RO - Halfword Data Transfer: register offset
 * HDT_IO - Halfword Data Transfer: immediate offset
 * SDT - Single Data Transfer
 * UNDEF - Undefined
 * BDT - Block Data Transfer
 * B - Branch
 * CDT -  Coprocessor Data Transfer
 * CDO - Coprocessor Data Operation
 * CRT - Coprocessor Register Transfer
 * INT - Software Interrupt
 */
typedef enum InstructionSetFormat {
    // not one of ARM's types - but I wanted to include an unknown case
    UNKNOWN_INSTRUCTION_FORMAT = 0,

    DP,
    MUL,
    MULL,
    SDS,
    BEX,
    HDT_RO,
    HDT_IO,
    SDT,
    UNDEF,
    BDT,
    B,
    CDT,
    CDO,
    CRT,
    INT,
} instruction_set_format_t;

#endif //  COMMON_H
