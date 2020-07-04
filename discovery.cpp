#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // will use condition code C set, 0010
    arm.set_condition_code_flag(C, 1);

    // source r4, dest r6
    arm.registers.r4 = 30;

    // sub with immediate value 12
    arm_instruction i1 = 0b00100010010001000110111100000011;
    arm.execute(i1);
    // 30 - 12 = 18
    //REQUIRE(arm.registers.r6 == 18);
    return 0;
}
