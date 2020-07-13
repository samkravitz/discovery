#include <iostream>
#include <fstream>
#include "cpu/arm_7tdmi.h"
#include "memory/memory.h"

void run_asm(char *name) {
    arm_7tdmi arm;
    arm.mem.load_rom(name);
    arm_instruction i;
    while (true) {
        i = arm.mem.get_instruction(arm.registers.r15);
        std::cout << i << "\n";
        arm.execute(i);
        if (i == 0) arm.registers.r15 += 4;
    }
}

int main(int argc, char **argv) {
    std::cout << "Gameboy emulator!" << "\n";
    run_asm(argv[1]);
    //arm_7tdmi arm;
    //Memory mem;

    // // source r1, dest r2
    // arm.registers.r1 = 100;
    // arm.registers.r11 = 0b10000000000000000000000001010101; // 7 digits

    // // reverse subtract carry with ASR with destination register 11 shifted 7 times (10)
    // // 0001 00 0 0111 1 0001 0010 00111 10 0 1011;
    // arm_instruction i1 = 0b00010000111100010010001111001011;
    // arm.execute(i1);

    // word result = 0b11111111000000000000000000000000 - 100 + 1 - 1;
    return 1 + 2;
}
