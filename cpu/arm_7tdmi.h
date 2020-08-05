/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: arm_7tdmi.h
 * DATE: July 13, 2020
 * DESCRIPTION: class definition for arm7tdmi
 */
#ifndef ARM_7TDMI_H
#define ARM_7TDMI_H

#include "common.h"
#include "../memory/memory.h"

// data type for special registers
union status_register {
    struct bits {
        state_t state : 5;
        u8 t : 1;
        u8 f : 1;
        u8 i : 1;
        u32 reserved : 19;
        u8 v : 1;
        u8 c : 1;
        u8 z : 1;
        u8 n : 1;
    } bits;
    u32 full;
};

class arm_7tdmi {
    public:
        arm_7tdmi();
        ~arm_7tdmi() {};
        
        Memory *mem;

        struct registers_struct {
            // general purpose registers
            u32 r0;
            u32 r1;
            u32 r2;
            u32 r3;
            u32 r4;
            u32 r5;
            u32 r6;
            u32 r7;
            u32 r8;
            u32 r9;
            u32 r10;
            u32 r11;
            u32 r12;
            u32 r13;
            u32 r14; // subroutine link registers
            u32 r15; // program counter

            // fiq registers
            u32 r8_fiq;
            u32 r9_fiq;
            u32 r10_fiq;
            u32 r11_fiq;
            u32 r12_fiq;
            u32 r13_fiq;
            u32 r14_fiq;

            // svc registers
            u32 r13_svc;
            u32 r14_svc;

            // abt registers
            u32 r13_abt;
            u32 r14_abt;

            // irq registers
            u32 r13_irq;
            u32 r14_irq;

            // und registers
            u32 r13_und;
            u32 r14_und;

            status_register cpsr;

            status_register spsr_fiq;
            status_register spsr_svc;
            status_register spsr_abt;
            status_register spsr_irq;
            status_register spsr_und;
        } registers;
        
        void fetch();
        void decode(u32);
        void execute(u32);
        
        // getters / setters
        uint8_t get_condition_code_flag(condition_code_flag_t);
        void set_condition_code_flag(condition_code_flag_t, uint8_t);

        state_t get_state() { return state; }
        void set_state(state_t s) { state = s; }

        cpu_mode_t get_mode() { return mode; }
        void set_mode(cpu_mode_t m) { mode = m; }

        u32 get_register(u32);
        void set_register(int reg, u32 val);

        // instruction execution
        void branch_exchange(u32);
        void branch_link(u32);
        void data_processing(u32);
        void multiply(u32);
        void psr_transfer(u32);
        void single_data_transfer(u32);
        void halfword_data_transfer(u32);
        void block_data_transfer(u32);
        void single_data_swap(u32);
        void software_interrupt(u32);

        // thumb instructions
        void move_shifted_register_thumb(u16);
        void add_sub_thumb(u16);
        void move_immediate_thumb(u16);
        void alu_thumb(u16);
        void hi_reg_ops_thumb(u16);
        void pc_rel_load_thumb(u16);

        // misc
        void update_flags_logical(u32, u8);
        void update_flags_addition(u32, u32, u32);
        void update_flags_subtraction(u32, u32, u32);
        u8 shift_register(u32, u32 &, u8);
        void increment_pc();
        bool condition_met(u32);
        void update_psr(bool, u32);

    private:
        u32 current_instruction;
        state_t state;
        cpu_mode_t mode;
}; 

#endif // ARM_7TDMI_H