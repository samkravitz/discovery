/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: Arm7.h
 * DATE: July 13, 2020
 * DESCRIPTION: class definition for arm7tdmi
 */

#pragma once

#include "Memory.h"
#include "util.h"
#include "PPU.h"
#include "APU.h"
#include "mmio.h"

class Arm7
{
public:
    Arm7(Memory *mem);
    
    Memory *mem;

    u32  pipeline[3];
    bool pipeline_full;
    bool in_interrupt;

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
    
    void fetch();
    void decode();
    int  execute(u32);

    void tick(u8, u8, u8);

    u32  getRegister(u32);
    void setRegister(u32, u32);

    // instruction execution
    void branchExchange(u32);
    void branchLink(u32);
    void dataProcessing(u32);
    void multiply(u32);
    void multiplyLong(u32);
    void psrTransfer(u32);
    void singleDataTransfer(u32);
    void halfwordDataTransfer(u32);
    void blockDataTransfer(u32);
    void singleDataSwap(u32);
    void softwareInterruptArm(u32);

    // thumb instructions
    void moveShiftedRegister(u16);
    void addSubtract(u16);
    void moveImmediate(u16);
    void aluThumb(u16);
    void hiRegisterOps(u16);
    void pcRelLoad(u16);
    void loadStoreRegOffset(u16);
    void loadStoreSignedHalfword(u16);
    void loadStoreImmediate(u16);
    void loadStoreHalfword(u16);
    void spRelLoadStore(u16);
    void loadAddress(u16);
    void addOffsetToSp(u16);
    void pushPop(u16);
    void multipleLoadStore(u16);
    void conditionalBranch(u16);
    void softwareInterruptThumb(u16);
    void unconditionalBranch(u16);
    void longBranchLink(u16);

    // software interrupts (swi)
    void swiSoftReset();
    void swiRegisterRamReset();
    void swiVBlankIntrWait();
    void swiDivision();
    void swiSqrt();
    void swiArctan2();
    void swiCpuSet();
    void swiObjAffineSet();
    void swiBitUnpack();
    void swiRLUnCompVRAM();

    // handle hardware interrupts
    void handleInterrupt();

    // getters / setters
    u8   getConditionCodeFlag(ConditionFlag);
    void setConditionCodeFlag(ConditionFlag, u8);

    Mode getMode();
    void setMode(Mode);

    State getState()           { return registers.cpsr.t == 1 ? State::THUMB : State::ARM; }
    void setState(State state) { registers.cpsr.t = state == State::THUMB ? 1 : 0; }
        
private:
    int cycles;
    u32 last_read_bios;  

    static constexpr u32 bios_read_state[4] =
    {
        0xE129F000, // 0xDC  + 8 startup 
        0xE25EF004, // 0x134 + 8 irq execute
        0xE55EC002, // 0x13C + 8 irq finish
        0xE3A02004, // 0x188 + 8 swi finish}
    };

    // safely interface with memory
    u32  read8(u32);
    u32  read16(u32, bool sign = false);
    u32  read32(u32, bool ldr = false);
    void write8(u32, u8);
    void write16(u32, u16);
    void write32(u32, u32);

    // misc
    void updateFlagsLogical(u32, u8);
    void updateFlagsAddition(u32, u32, u32);
    void updateFlagsSubtraction(u32, u32, u32);
    void incrementPC();
    void updateCPSR(u32, bool);
    void updateSPSR(u32, bool);
    bool conditionMet(Condition);
    bool memCheckRead(u32 &);
    bool memCheckWrite(u32 &);
    u8   barrelShift(u32, u32 &, u8);
    bool checkState();
    void print();
};