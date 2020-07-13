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
    //run_asm(argv[1]);
    Memory mem;
    word address = 0x1000;
    mem.write_u32(address, 0xABCDEFA0);
    mem.read_u8(address);
    mem.read_u32(address);


    return 0;
}
