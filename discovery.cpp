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
    arm_7tdmi arm6;
    // Rn = 10, registers list is r8, r9, r11
    // 1110 100 0 1 1 1 0 1010 0000101100000000
    arm_instruction i6 = 0b11101000111010100000101100000000;
    arm6.set_register(10, 0x1000);
    arm6.set_register(8, 1);
    arm6.set_register(9, 5);
    arm6.set_register(11, 7);
    arm6.set_state(FIQ);
    arm6.set_register(10, 0x1000);
    arm6.set_register(8, 2);
    arm6.set_register(9, 4);
    arm6.set_register(11, 6);
    arm6.execute(i6);
    // REQUIRE(arm6.registers.r10 == 0x1000);
    // REQUIRE(arm6.mem.read_u32(0x1000) == 1);
    // REQUIRE(arm6.mem.read_u32(0x1004) == 5);
    // REQUIRE(arm6.mem.read_u32(0x1008) == 7);

    return 0;
}
