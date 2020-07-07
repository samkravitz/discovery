#include <iostream>
#include <fstream>
#include "cpu/arm_7tdmi.h"

void run_asm(char *name) {
    arm_7tdmi arm;
    std::ifstream rom(name, std::ios::in | std::ios::binary);
    if (!rom) return;
    arm_instruction i;
    while (rom.read((char *) &i, 4)) {
        std::cout << i << "\n";
        arm.execute(i);
    }
}

int main(int argc, char **argv) {
    std::cout << "Gameboy emulator!" << "\n";
    run_asm(argv[1]);
}
