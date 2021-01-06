#include "catch.hpp"

#include "../common/cpu.h"
#include "../common/util.h"
#include "../arm_7tdmi.h"

TEST_CASE("AND", "[data_processing]") {
    arm_7tdmi arm;

    // will use condition code GE, N == V
    arm.set_condition_code_flag(N, 1);
    arm.set_condition_code_flag(V, 1);

    // source r3, dest r4
    arm.registers.r3 = 0b11110000111100001111000011110000;

    u32 i1 = 0b10100010000000110100001011111111;
    arm.execute(i1);
    REQUIRE(arm.registers.r4 == 0b11110000000000000000000000000000);
}

TEST_CASE("EOR", "[data_processing]") {
    arm_7tdmi arm;

    // will use condition code Z set, 0000
    arm.set_condition_code_flag(Z, 1);

    // source r2, dest r8
    arm.registers.r2 = 0b0110;

    u32 i1 = 0b00000010001000101000000000001111;
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
    u32 i1 = 0b00100010010001000110111100000011;
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
    u32 i1 = 0b00100010011001011010111100000011;
    arm.execute(i1);
    // 12 - 6 = 6
    REQUIRE(arm.registers.r10 == 6);
}

TEST_CASE("ADD", "[data_processing]") {
    arm_7tdmi arm;
    
    arm.registers.r1 = 4; // 0b0100
    arm.registers.r2 = 15; // 0b1111

    // condition N == V, 0b1010
    // op1 = r1 = 4
    // dest = r10
    // op2 = LSL #4 on r2 = 15, should be 240
    u32 i = 0b10100000100000011010001000000010;
    arm.execute(i);
    REQUIRE(arm.registers.r10 == 244);
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
    u32 i1 = 0b00100010101000011101100110000011;
    arm.execute(i1);

    REQUIRE(arm.registers.r13 == 100 + 2146304 + 1);
}

// 0010|00|1|0001|0|0010|1000|000000001111
TEST_CASE("SBC", "[data_processing]") {
    arm_7tdmi arm;

    // will use condition code Z clear, 0001s
    arm.set_condition_code_flag(C, 1);

    // source r1, dest r2
    arm.registers.r1 = 0b10010000110100010010000001101111;
    arm.registers.r7 = 0b10;

    // subtract carry with ROX with destination register 7 (10)
    u32 i1 = 0b00010000110100010010000001100111;
    arm.execute(i1);

    u32 result = 0b10010000110100010010000001101111 - 0b10000000000000000000000000000001;
    //REQUIRE(arm.registers.r2 == (result + 0 - 1));
}

TEST_CASE("RSC", "[data_processing]") {
    arm_7tdmi arm;

    // source r1, dest r2
    arm.registers.r1 = 100;
    arm.registers.r11 = 0b10000000000000000000000001010101; // 7 digits

    // reverse subtract carry with ASR with destination register 11 shifted 7 times (10)
    // 0001 00 0 0111 1 0001 0010 00111 10 0 1011;
    u32 i1 = 0b00010000111100010010001111001011;
    arm.execute(i1);

    u32 result = 0b11111111000000000000000000000000 - 100 + 1 - 1;
    REQUIRE(arm.get_register(2) == result);
}

TEST_CASE("TST", "[data_processing]") {
    arm_7tdmi arm;

    // source r9, shifted register r14
    arm.registers.r9 = 0b10000000000000000000000000000000;
    arm.registers.r14 = 0b10000000000000000000000000000000;

    // TST with ASR #32
    u32 i1 = 0b00010001000110010010000001001110;
    arm.execute(i1);

    REQUIRE(arm.get_condition_code_flag(C) == 1);
    REQUIRE(arm.get_condition_code_flag(Z) == 0);
    REQUIRE(arm.get_condition_code_flag(N) == 1);
}

TEST_CASE("TEQ", "[data_processing]") { }

TEST_CASE("CMP", "[data_processing]") {
    arm_7tdmi arm;

    // source r5
    arm.registers.r5 = 32;
    arm.registers.r14 = 0b10000000000000000000000000000000;

    // CMP with 32 - immediate val 32
    //  0b0001 00 1 1010 1 0101 0010 0000 00100000;
    u32 i1 = 0b00010011010101010010000000100000;
    arm.execute(i1);

    REQUIRE(arm.get_condition_code_flag(C) == 1);
    REQUIRE(arm.get_condition_code_flag(Z) == 1);
    REQUIRE(arm.get_condition_code_flag(V) == 0);
    REQUIRE(arm.get_condition_code_flag(N) == 0);
}

TEST_CASE("CMN", "[data_processing]") {
    arm_7tdmi arm;

    // source r5
    arm.registers.r5 = 0xFFFFFFFF;
    arm.registers.r14 = 0b11110000000000000000000000000000;
    arm.registers.r12 = 28;

    // CMP with 32 - immediate val 32
    //  0b0001 00 1 1011 1 0101 0010 1100 0011 1110;
    u32 i1 = 0b00010011011101010010110000111110;
    arm.execute(i1);

    REQUIRE(arm.get_condition_code_flag(C) == 1);
    REQUIRE(arm.get_condition_code_flag(Z) == 0);
    REQUIRE(arm.get_condition_code_flag(V) == 0);
    REQUIRE(arm.get_condition_code_flag(N) == 0);
}

TEST_CASE("ORR", "[data_processing]") {
    arm_7tdmi arm;

    // source r5, dest r14
    arm.registers.r5 = 0xFFFFFFFF;
    arm.registers.r14 = 0b11110000000000000000000000000000;
    arm.registers.r12 = 1;
    // CMP with 32 - immediate val 32
    //  0b0001 00 0 1100 1 0101 1110 0000 1010 1100;
    u32 i1 = 0b00010001100101011110000010101100;
    arm.execute(i1);

    REQUIRE(arm.get_condition_code_flag(C) == 1);
    REQUIRE(arm.get_condition_code_flag(Z) == 0);
    REQUIRE(arm.get_condition_code_flag(N) == 1);
    REQUIRE(arm.get_register(14) == 0xFFFFFFFF);
}

TEST_CASE("MOV", "[data_processing]") {
    arm_7tdmi arm;

    // dest r0
    // MOV immediate val 32 into r0
    // 0b0001 00 1 1101 0 0101 0000 0000 00010000;
    u32 i1 = 0b00010011101001010000000000100000;
    arm.execute(i1);

    REQUIRE(arm.get_condition_code_flag(C) == 0);
    REQUIRE(arm.get_condition_code_flag(Z) == 0);
    REQUIRE(arm.get_condition_code_flag(N) == 0);
    REQUIRE(arm.get_register(0) == 32);
}

TEST_CASE("BIC", "[data_processing]") { }

TEST_CASE("MVN", "[data_processing]") { }
