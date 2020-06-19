#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "arm_7tdmi.h"

class cpu {
    public:
        cpu();
        ~cpu() {};
        arm_7tdmi arm;

        void execute(arm_instruction);
        state_t getState() { return state; }
        cpu_mode_t getMode() { return mode; }

    private:
        state_t state;
        cpu_mode_t mode;
};


#endif // CPU_H