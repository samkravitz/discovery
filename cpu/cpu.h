#ifndef CPU_H
#define CPU_H

#include "instruction.h"
#include "common.h"
#include "arm_7tdmi.h"

class cpu {
    public:
        cpu();
        ~cpu() {};
        arm_7tdmi arm;

        void execute(Instruction);
        State_t getState() { return state; }
        Mode_t getMode() { return mode; }

    private:
        State_t state;
        Mode_t mode;
};


#endif // CPU_H