#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

// cpu state
typedef enum State {
    ARM,
    THUMB
} State_t;

// cpu mode 
typedef enum Mode {
    USR, // The normal ARM program execution state
    FIQ, // Designed to support a data transfer or channel process
    IRQ, // Used for general-purpose interrupt handling
    SVC, // Protected mode for the operating system
    ABT, // Entered after a data or instruction prefetch abort
    SYS, // A privileged user mode for the operating system
    UND // Entered when an undefined instruction is executed
} Mode_t;

// condition code flag of program status register
typedef enum ConditionCodeFlag {
    N, // 31st bit 
    Z, // 30th bit
    C, // 29th bit
    V // 28th bit
} ConditionCodeFlag_t;

typedef uint8_t Byte;
typedef uint16_t Halfword;
typedef uint32_t Word;

#endif //  COMMON_H
