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

    // fiq registers
    word r8_fiq;
    word r9_fiq;
    word r10_fiq;
    word r11_fiq;
    word r12_fiq;
    word r13_fiq;
    word r14_fiq;

    // svc registers
    word r13_svc;
    word r14_svc;

    // abt registers
    word r13_abt;
    word r14_abt;

    // irq registers
    word r13_irq;
    word r14_irq;

    // und registers
    word r13_und;
    word r14_und;

    // special registers
    word cpsr;
    word spsr_fiq;
    word spsr_svc;
    word spsr_abt;
    word spsr_irq;
    word spsr_und;
};

class arm_7tdmi {
    public:
        arm_7tdmi();
        ~arm_7tdmi() {};
        
        registers_struct registers;
        
        void fetch();
        void decode(arm_instruction);
        void execute(arm_instruction);
        
        // getters / setters
        uint8_t get_condition_code_flag(condition_code_flag_t);
        void set_condition_code_flag(condition_code_flag_t, uint8_t);

        state_t get_state() { return state; }
        void set_state(state_t s) { state = s; }

        cpu_mode_t get_mode() { return mode; }
        void set_mode(cpu_mode_t m) { mode = m; }

        word get_register(uint32_t);
        void set_register(int reg, word val);

        // instruction execution
        void branch_exchange(arm_instruction);
        void branch_link(arm_instruction);
        void data_processing(arm_instruction);
        void multiply(arm_instruction);
        void psr_transfer(arm_instruction);

        // misc
        void update_flags_logical(word, uint8_t);
        void update_flags_addition(word, word, word);
        void update_flags_subtraction(word, word, word);

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
        word current_instruction;
        state_t state;
        cpu_mode_t mode;
}; 

#endif // ARM_7TDMI_H