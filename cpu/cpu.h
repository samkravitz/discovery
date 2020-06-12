#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "arm.h"

class cpu {
    public:
        cpu();
        ~cpu() {};
        State_t getState() { return state; }
        Mode_t getMode() { return mode; }
        arm a;

    private:
        State_t state;
        Mode_t mode;
};


#endif // CPU_H