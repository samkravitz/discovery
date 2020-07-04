#include "catch.hpp"

#include "../cpu.h"
#include "../common.h"
#include "../util.h"
#include "../arm_7tdmi.h"

TEST_CASE("branch_exchange") {
    arm_7tdmi arm;

    // will use condition C set, V clear for the instruction condition
    arm.set_condition_code_flag(C, 1);

    // BXHI 9 (BEX cond=HI Rn=9)
    arm_instruction i1 = 0b10000001001011111111111100011001;

    arm.registers.r9 = 0xbeefbeef;

    arm.execute(i1);

    REQUIRE(arm.registers.r15 == 0xbeefbeef); // contents of r9 moved to r15
    REQUIRE(arm.get_mode() == THUMB); // because Rn (9) is odd

    arm.registers.r6 = 0xabcd;

    // BXHI 6 (BEX cond=HI Rn=6)
    arm_instruction i2 = 0b10000001001011111111111100010110;
    arm.execute(i2);
    REQUIRE(arm.registers.r15 == 0xabcd); // contents of r6 moved to r15
    REQUIRE(arm.get_mode() == ARM); // because Rn (6) is even

    // unrecognized condition (Rn = 15)
    arm_instruction i3 = 0b10000001001011111111111100011111;
    arm.execute(i3);
    REQUIRE(arm.registers.r15 == 0xabcd); // contents of r15 unchanged
    REQUIRE(arm.get_state() == UND); // because Rn was 15
    REQUIRE(arm.get_mode() == ARM); // mode unchanged
}

