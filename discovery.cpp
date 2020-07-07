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
    //run_asm(argv[1]);

    arm_7tdmi arm;

    // will use condition code C set, 0010
    arm.set_condition_code_flag(C, 1);
    arm.set_condition_code_flag(V, 1);
    arm.set_condition_code_flag(N, 1);

    // source r1, dest r13
    arm.registers.r1 = 100;

    // add carry with immediate value 2146304 and carry set
    arm_instruction i1 = 0b00100010101000011101100110000011;
    arm.execute(i1);

    //REQUIRE(arm.registers.r13 == 100 + 2146304 + 1);
}
