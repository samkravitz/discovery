#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // will use condition code GE, N == V
    arm.set_condition_code_flag(N, 1);
    arm.set_condition_code_flag(V, 1);

    // source r3, dest r4
    arm.registers.r3 = 0b11110000111100001111000011110000;

    arm_instruction i = 0b10100010000000110100001011111111;
    arm.execute(i);
    return 0;
}
