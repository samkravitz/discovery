#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // will use condition code Z clear, 0001s

    // source r1, dest r2
    arm.registers.r1 = 100;
    arm.registers.r11 = 0b10000000000000000000000001010101; // 7 digits

    // reverse subtract carry with ASR with destination register 11 shifted 7 times (10)
    // 0001 00 0 0111 1 0001 0010 00111 100 1011;
    arm_instruction i1 = 0b00010000111100010010001111001011;
    arm.execute(i1);

    word result = 0b11111111000000000000000000000000 - 100 + 1 - 1;
    //REQUIRE(arm.registers.r2 == result);
    return 0;
}
