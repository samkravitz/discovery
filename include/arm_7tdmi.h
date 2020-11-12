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

#include "cpu.h"
#include "memory.h"
#include "util.h"
#include "gpu.h"

// data type for special registers
union status_register
{
    struct bits
    {
        state_t state : 5;
        u8 t : 1;
        u8 f : 1;
        u8 i : 1;
        u32 reserved : 20;
        u8 v : 1;
        u8 c : 1;
        u8 z : 1;
        u8 n : 1;
    } bits;
    u32 full;
};

class arm_7tdmi
{
    public:
        arm_7tdmi();
        ~arm_7tdmi();
        
        Memory *mem;

        u32  pipeline[3];
        bool pipeline_full;
        bool in_interrupt;
        u32  current_interrupt;
        u32  cycles;

        struct registers
        {
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

        void cycle(u8, u8, u8);
        
        // getters / setters
        u8 get_condition_code_flag(condition_code_flag_t);
        void set_condition_code_flag(condition_code_flag_t, u8);

        state_t get_state();
        void set_state(state_t s); 

        cpu_mode_t get_mode() { return (cpu_mode_t) registers.cpsr.bits.t; }
        void set_mode(cpu_mode_t m) { registers.cpsr.bits.t = m; }

        u32 get_register(u32);
        void set_register(u32, u32);

        // instruction execution
        void branch_exchange(u32);
        void branch_link(u32);
        void data_processing(u32);
        void multiply(u32);
        void multiply_long(u32);
        void psr_transfer(u32);
        void single_data_transfer(u32);
        void halfword_data_transfer(u32);
        void block_data_transfer(u32);
        void single_data_swap(u32);
        void software_interrupt(u32);

        // thumb instructions
        void move_shifted_register(u16);
        void add_sub(u16);
        void move_immediate(u16);
        void alu_thumb(u16);
        void hi_reg_ops(u16);
        void pc_rel_load(u16);
        void load_store_reg(u16);
        void load_store_signed_halfword(u16);
        void load_store_immediate(u16);
        void load_store_halfword(u16);
        void sp_load_store(u16);
        void load_address(u16);
        void add_offset_to_sp(u16);
        void push_pop(u16);
        void multiple_load_store(u16);
        void conditional_branch(u16);
        void software_interrupt_thumb(u16);
        void unconditional_branch(u16);
        void long_branch_link(u16);

        // software interrupts (swi)
        void swi_softReset();
        void swi_registerRamReset();
        void swi_division();
        void swi_sqrt();
        void swi_arctan2();
        void swi_cpuSet();
        void swi_objAffineSet();
        void swi_bitUnpack();

        // handle hardware interrupts
        void handle_interrupt();

    private:
        // safely interface with memory
        u8 read_u8(u32);
        u32 read_u16(u32, bool);
        u32 read_u32(u32, bool);
        void write_u8(u32, u8);
        void write_u16(u32, u16);
        void write_u32(u32, u32);

        // misc
        void update_flags_logical(u32, u8);
        void update_flags_addition(u32, u32, u32);
        void update_flags_subtraction(u32, u32, u32);
        void increment_pc();
        void update_cpsr(u32, bool);
        void update_spsr(u32, bool);
        bool condition_met(condition_t);
        bool mem_check(u32 &);
        u8   barrel_shift(u32, u32 &, u8);
        bool check_state();
}; 

#endif // ARM_7TDMI_H