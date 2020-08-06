#include <iostream>
#include <fstream>
#include "discovery.h"
#include "cpu/arm_7tdmi.h"
#include "memory/memory.h"

discovery::discovery() {
    cpu.mem = new Memory();
    gpu.mem = cpu.mem;
}

void run_asm(char *name) {
    discovery emulator;
    emulator.cpu.mem->load_rom(name);
    u32 i;
    while (true) {
        emulator.cpu.fetch();
        i = emulator.cpu.current_instruction;
        std::cout << i << "\n";
        emulator.cpu.execute(i);
        emulator.gpu.draw();
    }
}

int main(int argc, char **argv) {
    std::cout << "Gameboy emulator!" << "\n";
    run_asm(argv[1]);
    discovery emulator;
    // while (true) {
    //     emulator.gpu.draw_pixel(120, 80);
    // }
    // arm_7tdmi arm;

    // // source r5, dest r14
    // arm.registers.r5 = 0xFFFFFFFF;
    // arm.registers.r14 = 0b11110000000000000000000000000000;
    // arm.registers.r12 = 1;
    // // CMP with 32 - immediate val 32
    // //  0b0001 00 0 1100 1 0101 1110 0000 1010 1100;
    // arm_instruction i1 = 0b00010001100101011110000010101100;
    // arm.execute(i1);

    // REQUIRE(arm.get_condition_code_flag(C) == 1);
    // REQUIRE(arm.get_condition_code_flag(Z) == 0);
    // REQUIRE(arm.get_condition_code_flag(N) == 1);
    // REQUIRE(arm.get_register(14) == 0xFFFFFFFF);
    return 0;
}
