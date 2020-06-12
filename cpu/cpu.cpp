#include "cpu.h"

// initialize mode and state of cpu
cpu::cpu() {
    mode = USR;
    state = ARM;
}