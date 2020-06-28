#ifndef ARM_7TDMI_H
#define ARM_7TDMI_H

#include "common.h"

struct registers_struct {
    // general purpose registers
    word r0;
    word r1;
    word r2;
    word r3;
    word r4;
    word r5;
    word r6;
    word r7;
    word r8;
    word r9;
    word r10;
    word r11;
    word r12;
    word r13;
    word r14; // subroutine link registers
    word r15; // program counter

    // special registers
    word cpsr;
    word spsr0;
    word spsr1;
    word spsr2;
    word spsr3;
    word spsr4;
};

class arm_7tdmi {
    public:
        arm_7tdmi();
        ~arm_7tdmi() {};
        void execute(arm_instruction);

        // getters / setters
        uint8_t get_condition_code_flag(condition_code_flag_t);
        void set_condition_code_flag(condition_code_flag_t, uint8_t);
        registers_struct get_registers() { return registers; }

    private:
        /* ARM state - 15 general purpose registers and 1 non-gp

         * R14 - is used as the subroutine link register. This receives a copy of
         * R15 when a Branch and Link (BL) instruction is executed. A
         * tall other times it may be treated as a general-purpose
         * register. The corresponding banked registers R14_svc,R14_irq, R14_fiq,
         * R14_abt and R14_und are similarly usedto hold the return values of R15 when interrupts and
         * exceptions arise, or when Branch and Link instructions are
         * executed within interrupt or exception routines
         * 
         * R15 holds the Program Counter (PC). In ARM state, bits [1:0] of
         * R15 are zero and bits [31:2] contain the PC.
         * In THUMB state,bit [0] is zero and bits [31:1] contain the PC.
         * 
         */
        registers_struct registers;
}; 

#endif // ARM_7TDMI_H