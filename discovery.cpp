#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;
    
    arm.registers.r1 = 4; // 0b0100
    arm.registers.r2 = 15; // 0b1111

    // condition N == V, 0b1010
    // op1 = r1 = 4
    // dest = r10
    // op2 = LSL #4 on r2 = 15, should be 240
    // 0b1010|00|0|0100|0|0001|1010|00100 000 0010;
    arm_instruction i = 0b10100000100000011010001000000010;
    arm.execute(i);
    //REQUIRE(arm.registers.r10 == 64);
    return 0;
}
