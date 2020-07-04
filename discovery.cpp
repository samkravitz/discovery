#include <iostream>
#include "cpu/arm_7tdmi.h"

int main() {
    std::cout << "Gameboy emulator!" << "\n";
    arm_7tdmi arm;

    // will use condition code C set, 0010
    arm.set_condition_code_flag(C, 1);
    arm.set_condition_code_flag(V, 1);
    arm.set_condition_code_flag(N, 1);

    // source r1, dest r13
    arm.registers.r1 = 100;

    // add carry with immediate value 2146304 and carry set
    //                       0010|00|1|0001|0|0010|1000|000000001111
    arm_instruction i1 = 0b00100010101000011101100110000011;
    arm.execute(i1);

    //REQUIRE(arm.registers.r13 == 100 + 2146304 + 1);
    return 0;
}
