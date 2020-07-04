#include "catch.hpp"

#include "../cpu.h"
#include "../common.h"
#include "../util.h"
#include "../arm_7tdmi.h"

TEST_CASE("AND", "[data_processing]") {
    arm_7tdmi arm;

    // will use condition code GE, N == V
    arm.set_condition_code_flag(N, 1);
    arm.set_condition_code_flag(V, 1);

    // source r3, dest r4
    arm.registers.r3 = 0b11110000111100001111000011110000;

    arm_instruction i1 = 0b10100010000000110100001011111111;
    arm.execute(i1);
    REQUIRE(arm.registers.r4 == 0b11110000000000000000000000000000);
}

TEST_CASE("EOR", "[data_processing]") {
    arm_7tdmi arm;

    // will use condition code Z set, 0000
    arm.set_condition_code_flag(Z, 1);

    // source r2, dest r8
    arm.registers.r2 = 0b0110;

    arm_instruction i1 = 0b00000010001000101000000000001111;
    arm.execute(i1);
    // 0b0110 XOR 0b1111 = 0b1001
    REQUIRE(arm.registers.r8 == 0b1001);
}

// instruction split up
// 0010|00|1|0001|0|0010|1000|000000001111
TEST_CASE("SUB", "[data_processing]") {
    arm_7tdmi arm;

    // will use condition code C set, 0010
    arm.set_condition_code_flag(C, 1);

    // source r4, dest r6
    arm.registers.r4 = 30;

    // sub with immediate value 12
    arm_instruction i1 = 0b00100010010001000110111100000011;
    arm.execute(i1);
    // 30 - 12 = 18
    REQUIRE(arm.registers.r6 == 18);
}

TEST_CASE("RSB", "[data_processing]") {
    arm_7tdmi arm;

    // will use condition code Z clear, 0001
    arm.set_condition_code_flag(C, 1);
    arm.set_condition_code_flag(V, 1);
    arm.set_condition_code_flag(N, 1);


    // source r5, dest r10
    arm.registers.r5 = 6;

    // sub with immediate value 12
    arm_instruction i1 = 0b00100010011001011010111100000011;
    arm.execute(i1);
    // 12 - 6 = 6
    REQUIRE(arm.registers.r10 == 6);
}

TEST_CASE("ADD", "[data_processing]") {
    
}

TEST_CASE("ADC", "[data_processing]") {
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

    REQUIRE(arm.registers.r13 == 100 + 2146304 + 1);
}