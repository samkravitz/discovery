#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "arm_7tdmi.h"
//#include "instruction.h"

class arm_7tdmi;

class cpu {
    public:
        cpu();
        ~cpu() {};
        arm_7tdmi arm;

        void execute();
        State_t getState() { return state; }
        Mode_t getMode() { return mode; }

    private:
        State_t state;
        Mode_t mode;
};


#endif // CPU_H