#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // source r5
    arm.registers.r5 = 32;
    arm.registers.r14 = 0b10000000000000000000000000000000;

    // CMP with 32 - immediate val 32
    //  0b0001 00 0 1010 1 0101 0010 0000 00100000;
    arm_instruction i1 = 0b00010011010101010010000000100000;
    arm.execute(i1);

    // REQUIRE(arm.get_condition_code_flag(C) == 0);
    // REQUIRE(arm.get_condition_code_flag(Z) == 1);
    // REQUIRE(arm.get_condition_code_flag(V) == 0);
    // REQUIRE(arm.get_condition_code_flag(N) == 0);
    return 0;
}
