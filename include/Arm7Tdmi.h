/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Arm7Tdmi.h
 * DATE: July 13, 2020
 * DESCRIPTION: class definition for arm7tdmi
 */

#pragma once

#include "Memory.h"
#include "util.h"
#include "PPU.h"
#include "APU.h"
#include "mmio.h"

class Arm7Tdmi
{
    public:
        Arm7Tdmi(Memory *mem);
        ~Arm7Tdmi();
        
        Memory *mem;

        u32  pipeline[3];
        bool pipeline_full;
        bool in_interrupt;
        u32  cycles;

        // data type for special registers
        struct StatusRegister
        {
            union
            {
                struct
                {
                    u8 mode      :  5;
                    u8 t         :  1;
                    u8 f         :  1;
                    u8 i         :  1;
                    u32 reserved : 20;
                    u8 v         :  1;
                    u8 c         :  1;
                    u8 z         :  1;
                    u8 n         :  1;
                };

                u32 raw;
            };
        };

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

            StatusRegister cpsr;
            StatusRegister spsr_fiq;
            StatusRegister spsr_svc;
            StatusRegister spsr_abt;
            StatusRegister spsr_irq;
            StatusRegister spsr_und;
        } registers;
        
        void Fetch();
        void Decode(u32);
        void Execute(u32);

        void Tick(u8, u8, u8);

        u32 GetRegister(u32);
        void SetRegister(u32, u32);

        // instruction execution
        void BranchExchange(u32);
        void BranchLink(u32);
        void DataProcessing(u32);
        void Multiply(u32);
        void MultiplyLong(u32);
        void PSRTransfer(u32);
        void SingleDataTransfer(u32);
        void HalfwordDataTransfer(u32);
        void BlockDataTransfer(u32);
        void SingleDataSwap(u32);
        void SoftwareInterruptArm(u32);

        // thumb instructions
        void MoveShiftedRegister(u16);
        void AddSubtract(u16);
        void MoveImmediate(u16);
        void AluThumb(u16);
        void HiRegisterOps(u16);
        void PcRelLoad(u16);
        void LoadStoreRegOffset(u16);
        void LoadStoreSignedHalfword(u16);
        void LoadStoreImmediate(u16);
        void LoadStoreHalfword(u16);
        void SpRelLoadStore(u16);
        void LoadAddress(u16);
        void AddOffsetToSp(u16);
        void PushPop(u16);
        void MultipleLoadStore(u16);
        void ConditionalBranch(u16);
        void SoftwareInterruptThumb(u16);
        void UnconditionalBranch(u16);
        void LongBranchLink(u16);

        // software interrupts (swi)
        void SwiSoftReset();
        void SwiRegisterRamReset();
        void SwiVBlankIntrWait();
        void SwiDivision();
        void SwiSqrt();
        void SwiArctan2();
        void SwiCpuSet();
        void SwiObjAffineSet();
        void SwiBitUnpack();
        void SwiRLUnCompVRAM();

        // handle hardware interrupts
        void HandleInterrupt();

        // getters / setters
        u8   GetConditionCodeFlag(ConditionFlag);
        void SetConditionCodeFlag(ConditionFlag, u8);

        Mode GetMode();
        void SetMode(Mode);

        State GetState()           { return registers.cpsr.t == 1 ? State::THUMB : State::ARM; }
        void SetState(State state) { registers.cpsr.t = state == State::THUMB ? 1 : 0; }
        
    private:
        // safely interface with memory
        u8   Read8(u32);
        u32  Read16(u32, bool);
        u32  Read32(u32, bool);
        void Write8(u32, u8);
        void Write16(u32, u16);
        void Write32(u32, u32);

        static constexpr u32 bios_read_state[4] =
        {
            0xE129F000, // 0xDC  + 8 startup 
            0xE25EF004, // 0x134 + 8 irq execute
            0xE55EC002, // 0x13C + 8 irq finish
            0xE3A02004, // 0x188 + 8 swi finish}
        };

        u32 last_read_bios;

        // misc
        void UpdateFlagsLogical(u32, u8);
        void UpdateFlagsAddition(u32, u32, u32);
        void UpdateFlagsSubtraction(u32, u32, u32);
        void IncrementPC();
        void UpdateCPSR(u32, bool);
        void UpdateSPSR(u32, bool);
        bool ConditionMet(Condition);
        bool MemCheckRead(u32 &);
        bool MemCheckWrite(u32 &);
        u8   BarrelShift(u32, u32 &, u8);
        bool CheckState();
};