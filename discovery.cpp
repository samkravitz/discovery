#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // source r9, shifted register r14
    arm.registers.r9 = 0b10000000000000000000000000000000;
    arm.registers.r14 = 0b10000000000000000000000000000000;

    // reverse subtract carry with ASR with destination register 11 shifted 7 times (10)
    // 0b0001 00 0 1000 1 1001 0010 00000 100 1110;
    arm_instruction i1 = 0b00010001000110010010000001001110;
    arm.execute(i1);

    // REQUIRE(arm.get_condition_code_flag(C) == 0);
    // REQUIRE(arm.get_condition_code_flag(Z) == 0);
    // REQUIRE(arm.get_condition_code_flag(V) == 1);
    return 0;
}
