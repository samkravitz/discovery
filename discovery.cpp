#include <iostream>
#include <fstream>
#include "cpu/arm_7tdmi.h"
#include "memory/memory.h"

void run_asm(char *name) {
    arm_7tdmi arm;
    Memory mem;
    mem.load_rom(name);
    arm_instruction i;
    while (true) {
        i = mem.get_instruction(arm.registers.r15);
        std::cout << i << "\n";
        arm.execute(i);
    }
}

int main(int argc, char **argv) {
    std::cout << "Gameboy emulator!" << "\n";
    run_asm(argv[1]);

}
