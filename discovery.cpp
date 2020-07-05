#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // source r5, dest r14
    arm.registers.r5 = 0xFFFFFFFF;
    arm.registers.r14 = 0b11110000000000000000000000000000;
    arm.registers.r12 = 1;
    // CMP with 32 - immediate val 32
    //  0b0001 00 1 1100 1 0101 1110 0000 1010 1100;
    arm_instruction i1 = 0b00010001100101011110000010101100;
    arm.execute(i1);

    // REQUIRE(arm.get_condition_code_flag(C) == 1);
    // REQUIRE(arm.get_condition_code_flag(Z) == 0);
    // REQUIRE(arm.get_condition_code_flag(N) == 1);
    // REQUIRE(arm.get_register(14) == 0xFFFFFFFF);
}
