#include "catch.hpp"

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
    REQUIRE(arm.get_mode() == THUMB); // because Rn[0] is odd

    arm.registers.r6 = 0xabcde;

    // BXHI 6 (BEX cond=HI Rn=6)
    arm_instruction i2 = 0b10000001001011111111111100010110;
    arm.execute(i2);
    REQUIRE(arm.registers.r15 == 0xabcde); // contents of r6 moved to r15
    REQUIRE(arm.get_mode() == ARM); // because Rn[0] is even

    // unrecognized condition (Rn = 15)
    arm_instruction i3 = 0b10000001001011111111111100011111;
    arm.execute(i3);
    REQUIRE(arm.registers.r15 == 0xabcde); // contents of r15 unchanged
    REQUIRE(arm.get_state() == UND); // because Rn was 15
    REQUIRE(arm.get_mode() == ARM); // mode unchanged
}

TEST_CASE("multiply") {
    arm_7tdmi arm;

    arm.set_condition_code_flag(Z, 1);

    // 0b0000000000asRdxxRnxxRsxx1001Rmxx
    // r3 = r1 * r2 => rd = rm * rs
    arm_instruction i1 = 0b00000000000000110000001010010111;
    arm.execute(i1);
    REQUIRE(arm.registers.r3 == 14);

    // r4 = r1 * r2 + 2
    arm_instruction i2 = 0b00000000001001000010001010010111;
    arm.execute(i2);
    REQUIRE(arm.registers.r4 == 16);
}

TEST_CASE("branch_link") {
    arm_7tdmi arm1;

    // 1110 101 0 000000000000000000000000 
    arm_instruction i1 = 0b11101010000000000000000000000101;

    // branch offset 20
    arm1.execute(i1);
    REQUIRE(arm1.registers.r15 == 20);

    arm_7tdmi arm2;
    arm2.registers.r15 = 100;
    arm_instruction i2 = 0b11101010111111111111111111110110; // offset should expand to -40 in 2s compliment

    // branch offset -40
    arm2.execute(i2);
    REQUIRE(arm2.registers.r15 == 60);
}

TEST_CASE("single_data_transfer") {
    // 1110 01 I P U B W L Rn Rd 000000000000
    arm_7tdmi arm;
    // base r0 dest r1
    arm.registers.r0 = 0x1000;
    arm.mem.write_u32(0x1000, 0xABCDEFA0);
    // LDR R1, 0x1000
    // Load value of address 0x1000 with 0 immediate offset to register 1
    arm_instruction i = 0b11100111100100000001000000000000;
    arm.execute(i);

    REQUIRE(arm.registers.r1 == 0xABCDEFA0);

    arm_7tdmi arm2;
    // base r3 source r4 Rm r7
    arm2.registers.r3 = 0;
    arm2.registers.r4 = 0xBEEFBEEF;
    arm2.registers.r7 = 0b1111;
    // STRB r4, r3 where offset comes from register 7 (1111), LSL 4 times to become 0xf0
    arm_instruction i2 = 0b11100101110000110100001000000111;
    arm2.execute(i2);
    REQUIRE(arm2.mem.read_u8(0xf0) == 0xEF);
}

TEST_CASE("halfword_data_transfer") {
    // TEST 1 - LOAD SIGNED HALFWORD
    // 1110 000 P U I W L Rn Rd 0000 1SH1 Rm
    arm_7tdmi arm1;
    // base r10 dest r11 offset r12
    arm1.registers.r10 = 0x1000;
    arm1.registers.r12 = 0x1000;
    arm1.mem.write_u16(0x1000, 0b1111000011110000); // memory address 0x1000 = 0b1111000011110000
    // 1110 000 0 1 0 0 1 1010 1011 0000 1111 1100
    // load signed halfword from address 0x1000 into r11
    arm_instruction i1 = 0b11100000100110101011000011111100;
    arm1.execute(i1);
    REQUIRE(arm1.registers.r11 == 0b11111111111111111111000011110000); // r11 should be sign extended with 1s
    REQUIRE(arm1.registers.r10 == 0x2000); // was a post index, so write back is guaranteed

    // TEST 2 - STORE UNSIGNED HALFWORD
    arm_7tdmi arm2;
    // base r1 source r2
    arm2.registers.r1 = 0xABCD;
    arm2.registers.r2 = 0x1001;
    // 1110 000 1 1 1 0 0 0001 0010 1111 1011 0000
    // store unsigned halfword from r2 into Address ABCD + immediate offset 0b11110000 (pre-index)
    arm_instruction i2 = 0b11100001110000010010111110110000;
    arm2.execute(i2);
    REQUIRE(arm2.mem.read_u16(0xABCD + 0b11110000) == 0x1001);
    REQUIRE(arm2.registers.r1 == 0xABCD); // was a pre index with no writeback

    // TEST 3 - LOAD SIGNED BYTE
    arm_7tdmi arm3;
    // base r6, dest r7, offset r8
    arm3.registers.r6 = 0x1004;    
    arm3.registers.r8 = 4;
    arm3.mem.write_u16(0x1000, 124); // memory address 0x1000 = 155
    // 1110 000 1 0 0 0 1 0110 0111 0000 1101 1000
    // load signed byte (124) from address 0x1004 - offset address (r8)
    arm_instruction i3 = 0b11100001000101100111000011011000;
    arm3.execute(i3);
    REQUIRE(arm3.registers.r7 == 124); // r7 should be sign extended with 0s
    REQUIRE(arm3.registers.r6 == 0x1004); // was a pre index with no writeback
}