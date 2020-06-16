#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "common.h"
#include "cpu.h"

// 32 bit ARM instruction
typedef Word Instruction;

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
} Condition_t;

// determine if the condition field of an instruction is true, given the state of the CPSR
bool isConditionMet(Instruction, arm &);


#endif // INSTRUCTION_H