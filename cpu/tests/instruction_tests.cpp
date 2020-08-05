#include "catch.hpp"

#include "../common.h"
#include "../util.h"
#include "../arm_7tdmi.h"

TEST_CASE("branch_exchange") {
    arm_7tdmi arm;

    // will use condition C set, V clear for the instruction condition
    arm.set_condition_code_flag(C, 1);

    // BXHI 9 (BEX cond=HI Rn=9)
    u32 i1 = 0b10000001001011111111111100011001;

    arm.registers.r9 = 0xbeefbeef;

    arm.execute(i1);

    REQUIRE(arm.registers.r15 == 0xbeefbeef - 1); // contents of r9 moved to r15
    REQUIRE(arm.get_mode() == THUMB); // because Rn[0] is odd

    arm_7tdmi arm2;
    arm2.registers.r6 = 0xabcde;

    // BXHI 6 (BEX cond=HI Rn=6)
    u32 i2 = 0b11100001001011111111111100010110;
    arm2.execute(i2);
    REQUIRE(arm2.registers.r15 == 0xabcde); // contents of r6 moved to r15
    REQUIRE(arm2.get_mode() == ARM); // because Rn[0] is even

    // unrecognized condition (Rn = 15)
    u32 i3 = 0b11100001001011111111111100011111;
    arm2.execute(i3);
    REQUIRE(arm2.registers.r15 == 0xabcde); // contents of r15 unchanged
    REQUIRE(arm2.get_state() == UND); // because Rn was 15
    REQUIRE(arm2.get_mode() == ARM); // mode unchanged
}

TEST_CASE("multiply") {
    arm_7tdmi arm;

    arm.set_condition_code_flag(Z, 1);

    // 0b0000000000asRdxxRnxxRsxx1001Rmxx
    // r3 = r1 * r2 => rd = rm * rs
    u32 i1 = 0b00000000000000110000001010010111;
    arm.execute(i1);
    REQUIRE(arm.registers.r3 == 14);

    // r4 = r1 * r2 + 2
    u32 i2 = 0b00000000001001000010001010010111;
    arm.execute(i2);
    REQUIRE(arm.registers.r4 == 16);
}

TEST_CASE("branch_link") {
    arm_7tdmi arm1;

    // 1110 101 0 000000000000000000000000 
    u32 i1 = 0b11101010000000000000000000000101;

    // branch offset 20
    arm1.execute(i1);
    REQUIRE(arm1.registers.r15 == 28);

    arm_7tdmi arm2;
    arm2.registers.r15 = 100;
    u32 i2 = 0b11101010111111111111111111110110; // offset should expand to -40 in 2s compliment

    // branch offset -40
    arm2.execute(i2);
    REQUIRE(arm2.registers.r15 == 68);
}

TEST_CASE("single_data_transfer") {
    // 1110 01 I P U B W L Rn Rd 000000000000
    arm_7tdmi arm;
    // base r0 dest r1
    arm.registers.r0 = 0x1000;
    arm.mem->write_u32(0x1000, 0xABCDEFA0);
    // LDR R1, 0x1000
    // Load value of address 0x1000 with 0 immediate offset to register 1
    u32 i = 0b11100101100100000001000000000000;
    arm.execute(i);

    REQUIRE(arm.registers.r1 == 0xABCDEFA0);

    arm_7tdmi arm2;
    // base r3 source r4 Rm r7
    arm2.registers.r3 = 0;
    arm2.registers.r4 = 0xBEEFBEEF;
    arm2.registers.r7 = 0b1111;
    // STRB r4, r3 where offset comes from register 7 (1111), LSL 4 times to become 0xf0
    u32 i2 = 0b11100111110000110100001000000111;
    arm2.execute(i2);
    REQUIRE(arm2.mem->read_u8(0xf0) == 0xEF);
}

TEST_CASE("halfword_data_transfer") {
    // TEST 1 - LOAD SIGNED HALFWORD
    // 1110 000 P U I W L Rn Rd 0000 1SH1 Rm
    arm_7tdmi arm1;
    // base r10 dest r11 offset r12
    arm1.registers.r10 = 0x1000;
    arm1.registers.r12 = 0x1000;
    arm1.mem->write_u16(0x1000, 0b1111000011110000); // memory address 0x1000 = 0b1111000011110000
    // 1110 000 0 1 0 0 1 1010 1011 0000 1111 1100
    // load signed halfword from address 0x1000 into r11
    u32 i1 = 0b11100000100110101011000011111100;
    arm1.execute(i1);
    REQUIRE(arm1.registers.r11 == 0b11111111111111111111000011110000); // r11 should be sign extended with 1s
    REQUIRE(arm1.registers.r10 == 0x2000); // was a post index, so write back is guaranteed

    // TEST 2 - STORE UNSIGNED HALFWORD
    arm_7tdmi arm2;
    // base r1 source r2
    arm2.registers.r1 = 0x2020202;
    arm2.registers.r2 = 0x1001;
    // 1110 000 1 1 1 0 0 0001 0010 1111 1011 0000
    // store unsigned halfword from r2 into Address 2020202 + immediate offset 0b11110000 (pre-index)
    u32 i2 = 0b11100001110000010010111110110000;
    arm2.execute(i2);
    REQUIRE(arm2.mem->read_u16(0x2020202 + 0b11110000) == 0x1001);
    REQUIRE(arm2.registers.r1 == 0x2020202); // was a pre index with no writeback

    // TEST 3 - LOAD SIGNED BYTE
    arm_7tdmi arm3;
    // base r6, dest r7, offset r8
    arm3.registers.r6 = 0x1004;    
    arm3.registers.r8 = 4;
    arm3.mem->write_u16(0x1000, 124); // memory address 0x1000 = 155
    // 1110 000 1 0 0 0 1 0110 0111 0000 1101 1000
    // load signed byte (124) from address 0x1004 - offset address (r8)
    u32 i3 = 0b11100001000101100111000011011000;
    arm3.execute(i3);
    REQUIRE(arm3.registers.r7 == 124); // r7 should be sign extended with 0s
    REQUIRE(arm3.registers.r6 == 0x1004); // was a pre index with no writeback
}

TEST_CASE("block_data_transfer") {
    // 1110 100 P U S W L Rn 0000000000000000
    // TEST 1 - STM POST-INCREMENT
    arm_7tdmi arm1;
    // Rn = 10, registers list is r1, r5, r7
    arm1.registers.r10 = 0x1000;
    arm1.registers.r1 = 1;
    arm1.registers.r5 = 5;
    arm1.registers.r7 = 7;
    // 1110 100 0 1 0 1 0 1010 0000000010100010
    u32 i1 = 0b11101000101010100000000010100010;
    arm1.execute(i1);
    REQUIRE(arm1.registers.r10 == 0x100c);
    REQUIRE(arm1.mem->read_u32(0x1000) == 1);
    REQUIRE(arm1.mem->read_u32(0x1004) == 5);
    REQUIRE(arm1.mem->read_u32(0x1008) == 7);

    // TEST 2 - STM PRE-INCREMENT
    arm_7tdmi arm2;
    // Rn = 10, registers list is r1, r5, r7
    arm2.registers.r10 = 0x1000;
    arm2.registers.r1 = 1;
    arm2.registers.r5 = 5;
    arm2.registers.r7 = 7;
    // 1110 100 1 1 0 1 0 1010 0000000010100010
    u32 i2 = 0b11101001101010100000000010100010;
    arm2.execute(i2);
    REQUIRE(arm2.registers.r10 == 0x100c);
    REQUIRE(arm2.mem->read_u32(0x1004) == 1);
    REQUIRE(arm2.mem->read_u32(0x1008) == 5);
    REQUIRE(arm2.mem->read_u32(0x100c) == 7);

    // TEST 3 - STM POST-DECREMENT
    arm_7tdmi arm3;
    // Rn = 10, registers list is r1, r5, r7
    arm3.registers.r10 = 0x1000;
    arm3.registers.r1 = 1;
    arm3.registers.r5 = 5;
    arm3.registers.r7 = 7;
    // 1110 100 0 0 0 1 0 1010 0000000010100010
    u32 i3 = 0b11101000001010100000000010100010;
    arm3.execute(i3);
    REQUIRE(arm3.registers.r10 == 0x0ff4);
    REQUIRE(arm3.mem->read_u32(0x1000) == 7);
    REQUIRE(arm3.mem->read_u32(0x0ffc) == 5);
    REQUIRE(arm3.mem->read_u32(0x0ff8) == 1);

    // TEST 4 - STM PRE-DECREMENT
    arm_7tdmi arm4;
    // Rn = 10, registers list is r1, r5, r7
    arm4.registers.r10 = 0x1000;
    arm4.registers.r1 = 1;
    arm4.registers.r5 = 5;
    arm4.registers.r7 = 7;
    // 1110 100 1 0 0 1 0 1010 0000000010100010
    u32 i4 = 0b11101001001010100000000010100010;
    arm4.execute(i4);
    REQUIRE(arm4.registers.r10 == 0x0ff4);
    REQUIRE(arm4.mem->read_u32(0x0ffc) == 7);
    REQUIRE(arm4.mem->read_u32(0x0ff8) == 5);
    REQUIRE(arm4.mem->read_u32(0x0ff4) == 1);

    // TEST 5 - LDM PRE-INCREMENT
    // Rn = 9, registers list is r2, r3, r4
    arm1.registers.r9 = 0x01000;

    // 1110 100 1 1 0 1 1 1001 0000000000011100
    u32 i5 = 0b11101000101110010000000000011100;
    arm1.execute(i5);
    REQUIRE(arm1.registers.r2 == 1);
    REQUIRE(arm1.registers.r3 == 5);
    REQUIRE(arm1.registers.r4 == 7);

    // TEST 6 - STM POST-INCREMENT With S bit set
    arm_7tdmi arm6;
    // Rn = 10, registers list is r8, r9, r11
    // 1110 100 0 1 1 1 0 1010 0000101100000000
    u32 i6 = 0b11101000111010100000101100000000;
    arm6.set_register(10, 0x1000);
    arm6.set_register(8, 1);
    arm6.set_register(9, 5);
    arm6.set_register(11, 7);
    arm6.set_state(FIQ);
    arm6.set_register(10, 0x1000);
    arm6.set_register(8, 2);
    arm6.set_register(9, 4);
    arm6.set_register(11, 6);
    arm6.execute(i6);
    REQUIRE(arm6.registers.r10 == 0x1000);
    REQUIRE(arm6.mem->read_u32(0x1000) == 1);
    REQUIRE(arm6.mem->read_u32(0x1004) == 5);
    REQUIRE(arm6.mem->read_u32(0x1008) == 7);
}

TEST_CASE("single_data_swap") {
    // TEST 1 - SWP WORD
    arm_7tdmi arm1;
    // r0 base register r1 source, r2 dest
    arm1.registers.r0 = 0x1000;
    arm1.registers.r1 = 0xFF11FF11;
    arm1.mem->write_u32(0x1000, 0xAA22AA22);

    // 1110 00010 0 00 0000 0010 0000 1001 0001;
    u32 i1 = 0b11100001000000000010000010010001;
    arm1.execute(i1);
    REQUIRE(arm1.registers.r2 == 0xAA22AA22);
    REQUIRE(arm1.mem->read_u32(0x1000) == 0xFF11FF11);

    // TEST 2 - SWP BYTE
    arm_7tdmi arm2;
    // r0 base register r1 source, r2 dest
    arm2.registers.r0 = 0x1000;
    arm2.registers.r1 = 0xFF11FF11;
    arm2.mem->write_u32(0x1000, 0xAA22AA22);

    // 1110 00010 1 00 0000 0010 0000 1001 0001;
    u32 i2 = 0b11100001010000000010000010010001;
    arm2.execute(i2);
    REQUIRE(arm2.registers.r2 == 0x22);
    REQUIRE(arm2.mem->read_u8(0x1000) == 0x11);
}

TEST_CASE("move_shifted_register_thumb") {
    // TEST 1 - LSL
    arm_7tdmi arm1;
    arm1.set_mode(THUMB);

    // Rs = 0 Rd = 2
    arm1.registers.r0 = 2;
    u16 i1 = 0b0000000100000010; // LSL by 4
    arm1.execute(i1);
    REQUIRE(arm1.registers.r2 == 32);

    // TEST 2 - LSR
    arm_7tdmi arm2;
    arm2.set_mode(THUMB);

    // Rs = 0 Rd = 2
    arm2.registers.r0 = 0b11111;
    u16 i2 = 0b0000100010000010; // LSR by 2
    arm2.execute(i2);
    REQUIRE(arm2.registers.r2 == 0b111);
    REQUIRE(arm2.get_condition_code_flag(C) == 1); // carry out from shifter

    // TEST 3 - ASR
    arm_7tdmi arm3;
    arm3.set_mode(THUMB);

    // Rs = 0 Rd = 2
    arm3.registers.r0 = 0b10000000000000000000000000001111;
    u16 i3 = 0b0001000010000010; // ASR by 2
    arm3.execute(i3);
    REQUIRE(arm3.registers.r2 == 0b11100000000000000000000000000011);
    REQUIRE(arm3.get_condition_code_flag(C) == 1); // carry out from shifter
    REQUIRE(arm3.get_condition_code_flag(N) == 1); // bit 31 is 1
}