#include <iostream>
#include <fstream>
#include "cpu/arm_7tdmi.h"
#include "memory/memory.h"

void run_asm(char *name) {
    arm_7tdmi arm;
    arm.mem.load_rom(name);
    arm_instruction i;
    while (true) {
        i = arm.mem.read_u32(arm.registers.r15);
        std::cout << i << "\n";
        arm.execute(i);
        if (i == 0) arm.registers.r15 += 4;
    }
}

int main(int argc, char **argv) {
    std::cout << "Gameboy emulator!" << "\n";
    //run_asm(argv[1]);
    arm_7tdmi arm3;
    // Rn = 10, registers list is r1, r5, r7
    arm3.registers.r10 = 0x1000;
    arm3.registers.r1 = 1;
    arm3.registers.r5 = 5;
    arm3.registers.r7 = 7;
    // 1110 100 0 0 0 1 0 1010 0000000010100010
    arm_instruction i3 = 0b11101000001010100000000010100010;
    arm3.execute(i3);


    return 0;
}
