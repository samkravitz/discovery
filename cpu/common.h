/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: common.h
 * DATE: June 27, 2020
 * DESCRIPTION: common typedefs throughout arm7tdmi use
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t halfword;
typedef uint32_t word;
typedef uint32_t arm_instruction;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// cpu state
typedef enum State {
    USR, // The normal ARM program execution state
    FIQ, // Designed to support a data transfer or channel process
    IRQ, // Used for general-purpose interrupt handling
    SVC, // Protected mode for the operating system
    ABT, // Entered after a data or instruction prefetch abort
    SYS, // A privileged user mode for the operating system
    UND // Entered when an undefined instruction is executed
} state_t;

// cpu mode 
typedef enum Mode {
    ARM,
    THUMB
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
 * DP - Data Processing
 * PSR - PSR Transfer
 * MUL - Multiply 
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
typedef enum InstructionSetFormat {
    // not one of ARM's types - but I wanted to include an unknown case
    UNKNOWN_INSTRUCTION_FORMAT = 0,

    DP,
    PSR,
    MUL,
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
} instruction_set_format_t;

typedef enum DataProcessingOpcodes {
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
} dp_opcodes_t;

#endif //  COMMON_H
