#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // will use condition code Z clear, 0001s
    arm.set_condition_code_flag(C, 1);

    // source r1, dest r2
    arm.registers.r1 = 0b10010000110100010010000001101111;
    arm.registers.r7 = 0b10;

    // add carry with immediate value 2146304 and carry set
    // 0001 00 0 0110 1000 1 0010 0000 0110 0111;
    arm_instruction i1 = 0b00010000110100010010000001100111;
    arm.execute(i1);

    word result = 0b10010000110100010010000001101111 - 0b10000000000000000000000000000001;
    //REQUIRE(arm.registers.r2 == (result + 0 - 1));
    return 0;
}
