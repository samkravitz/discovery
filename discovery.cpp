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
    arm_7tdmi arm1;
    // Rn = 10, registers list is r1, r5, r7
    arm1.registers.r10 = 0x1000;
    arm1.registers.r1 = 1;
    arm1.registers.r5 = 5;
    arm1.registers.r7 = 7;
    // 1110 100 0 1 0 1 0 1010 0000000010100010
    arm_instruction i1 = 0b11101000101010100000000010100010;
    arm1.execute(i1);

    arm1.registers.r9 = 0x0ffc;

    // 1110 100 1 1 0 1 1 1001 0000000000011100
    arm_instruction i5 = 0b11101000101110010000000000011100;
    arm1.execute(i5);


    return 0;
}
