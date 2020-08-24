#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <bitset>

#include "../arm_7tdmi.h"
#include "../common.h"
#include "../util.h"
#include "../../memory/memory.h"

TEST_CASE("ConditionFlagTests", "[condition_flag]") {
    arm_7tdmi arm;
    
    // REQUIRE all flags are 0 upon initialization
    REQUIRE(arm.get_condition_code_flag(N) == 0);
    REQUIRE(arm.get_condition_code_flag(Z) == 0);
    REQUIRE(arm.get_condition_code_flag(C) == 0);
    REQUIRE(arm.get_condition_code_flag(V) == 0);

    // set N, C, and V flags to 1
    arm.set_condition_code_flag(N, 1);
    arm.set_condition_code_flag(C, 1);
    arm.set_condition_code_flag(V, 1);

    // REQUIRE N, C, and V flags are now 1 and Z is still 0
    REQUIRE(arm.get_condition_code_flag(N) == 1);
    REQUIRE(arm.get_condition_code_flag(C) == 1);
    REQUIRE(arm.get_condition_code_flag(V) == 1);
    REQUIRE(arm.get_condition_code_flag(Z) == 0);

    // now set Z flag to 1 and REQUIRE it got changed
    arm.set_condition_code_flag(Z, 1);
    REQUIRE(arm.get_condition_code_flag(Z) == 1);

    // set N back to 0 and REQUIRE it got changed
    arm.set_condition_code_flag(N, 0);
    REQUIRE(arm.get_condition_code_flag(N) == 0);
    
    // try setting the code of a non recognized flag (should print some error message)
    arm.set_condition_code_flag((condition_code_flag_t) 9, 1);
    // try setting the code of a non recognized bit (should print some error message)
    arm.set_condition_code_flag(Z, 4);
}

TEST_CASE("InstructionConditionField", "[instruction_condition_field]") {
    condition_t z_set                       =      (condition_t) 0b0000; // 32 bits, Z Set
    condition_t z_clear                     =      (condition_t) 0b0001;
    condition_t c_set                       =      (condition_t) 0b0010;
    condition_t c_clear                     =      (condition_t) 0b0011;
    condition_t n_set                       =      (condition_t) 0b0100;
    condition_t n_clear                     =      (condition_t) 0b0101;
    condition_t v_set                       =      (condition_t) 0b0110;
    condition_t v_clear                     =      (condition_t) 0b0111;
    condition_t c_set_and_z_clear           =      (condition_t) 0b1000;
    condition_t c_clear_or_z_set            =      (condition_t) 0b1001;
    condition_t n_equal_v                   =      (condition_t) 0b1010;
    condition_t n_not_equal_v               =      (condition_t) 0b1011;
    condition_t z_clear_and_n_equal_v       =      (condition_t) 0b1100;
    condition_t z_set_or_n_not_equal_v      =      (condition_t) 0b1101;
    condition_t always                      =      (condition_t) 0b1110;
    condition_t noop                        =      (condition_t) 0b1111;

    // a couple of fresh cpu instances to test with
    arm_7tdmi z;
    arm_7tdmi c;
    arm_7tdmi n;
    arm_7tdmi v;

    // z 
    REQUIRE(z.condition_met(z_set) == false);
    REQUIRE(z.condition_met(z_clear) == true);
    z.set_condition_code_flag(Z, 1);
    REQUIRE(z.condition_met(z_set) == true);

    // c 
    REQUIRE(c.condition_met(c_set) == false);
    REQUIRE(c.condition_met(c_clear) == true);
    c.set_condition_code_flag(C, 1);
    REQUIRE(c.condition_met(c_set) == true);

    // n 
    REQUIRE(n.condition_met(n_set) == false);
    REQUIRE(n.condition_met(n_clear) == true);
    n.set_condition_code_flag(N, 1);
    REQUIRE(n.condition_met(n_set) == true);

    // v 
    REQUIRE(v.condition_met(v_set) == false);
    REQUIRE(v.condition_met(v_clear) == true);
    v.set_condition_code_flag(V, 1);
    REQUIRE(v.condition_met(v_set) == true);

    // c set and z clear
    REQUIRE(z.condition_met(c_set_and_z_clear) == false);
    z.set_condition_code_flag(Z, 0);
    z.set_condition_code_flag(C, 1);
    REQUIRE(z.condition_met(c_set_and_z_clear) == true);

    // c clear or z set
    REQUIRE(z.condition_met(c_clear_or_z_set) == false);
    z.set_condition_code_flag(Z, 1);
    REQUIRE(z.condition_met(c_clear_or_z_set) == true);
    z.set_condition_code_flag(Z, 0);
    z.set_condition_code_flag(C, 0);
    REQUIRE(z.condition_met(c_clear_or_z_set) == true);
    z.set_condition_code_flag(Z, 1);
    REQUIRE(z.condition_met(c_clear_or_z_set) == true);

    // n equals v
    REQUIRE(n.condition_met(n_equal_v) == false);
    REQUIRE(n.condition_met(n_not_equal_v) == true);
    n.set_condition_code_flag(V, 1);
    REQUIRE(n.condition_met(n_equal_v) == true);
    REQUIRE(n.condition_met(n_not_equal_v) == false);

    // z clear and (n equal v)
    REQUIRE(n.condition_met(z_clear_and_n_equal_v) == true);
    n.set_condition_code_flag(Z, 1);
    REQUIRE(n.condition_met(z_clear_and_n_equal_v) == false);

    // make a new cpu because this is getting confusing
    arm_7tdmi arm;

    // z set or (n not equal v)
    REQUIRE(arm.condition_met(z_set_or_n_not_equal_v) == false);
    arm.set_condition_code_flag(Z, 1);
    REQUIRE(arm.condition_met(z_set_or_n_not_equal_v) == true);
    arm.set_condition_code_flag(N, 1);
    REQUIRE(arm.condition_met(z_set_or_n_not_equal_v) == true);
    arm.set_condition_code_flag(Z, 0);
    REQUIRE(arm.condition_met(z_set_or_n_not_equal_v) == true);

    // always
    REQUIRE(arm.condition_met(always) == true);
    REQUIRE(z.condition_met(always) == true);
    REQUIRE(c.condition_met(always) == true);
    REQUIRE(v.condition_met(always) == true);
    REQUIRE(n.condition_met(always) == true);

    // never
    REQUIRE(arm.condition_met(noop) == false);
    REQUIRE(z.condition_met(noop) == false);
    REQUIRE(c.condition_met(noop) == false);
    REQUIRE(v.condition_met(noop) == false);
    REQUIRE(n.condition_met(noop) == false);
}

// #TODO - test cases for Instruction Set Format
// TEST_CASE("InstructionSetFormat", "[instruction_set_format]") {
//     u32 i = 0;
    
//     // zero'd instruction is actually a valid data processing instruction
//     REQUIRE(util::get_instruction_format(i) == DP);

//     // see docs/u32_set_bitfield.png to see which bits should be set for which instruction formats
//     //  (one's digit of instruction) -                    10987654321098765432109876543210
//     u32 dataProcessing               =      0b00000000000000000000000000000000;
//     u32 multiply                     =      0b00000000000000000000000010010000;
//     u32 multiplyLong                 =      0b00000000100000000000000010010000;
//     u32 singleDataSwap               =      0b00000001000000000000000010010000;
//     u32 branchExchange               =      0b00000001001011111111111100010000;
//     u32 hdtRegisterOffset            =      0b00000000000000000000000010010000;
//     u32 hdtImmediateOffset           =      0b00000000010000000000000010010000;
//     u32 singleDataTransfer           =      0b00000100000000000000000000000000;
//     u32 undefined                    =      0b00000110000000000000000000010000;
//     u32 blockDataTransfer            =      0b00001000000000000000000000000000;
//     u32 branch                       =      0b00001010000000000000000000000000;
//     u32 coprocessorDataTransfer      =      0b00001100000000000000000000000000;
//     u32 coprocessorDataOperation     =      0b00001110000000000000000000000000;
//     u32 coprocessorRegisterTransfer  =      0b00001110000000000000000000010000;
//     u32 softwareInterrupt            =      0b00001111000000000000000000000000;

//     // comparing these hypothetical instructions with their truth,
//     // in somewhat random order to make sure order is irrelvant for this function
//     REQUIRE(util::get_instruction_format(hdtImmediateOffset) == HDT_IO);
//     REQUIRE(util::get_instruction_format(branchExchange) == BEX);
//     REQUIRE(util::get_instruction_format(softwareInterrupt) == INT);
//     REQUIRE(util::get_instruction_format(singleDataSwap) == SDS);
//     REQUIRE(util::get_instruction_format(multiply) == MUL);
//     REQUIRE(util::get_instruction_format(coprocessorRegisterTransfer) == CRT);
//     REQUIRE(util::get_instruction_format(undefined) == UNDEF);
//     REQUIRE(util::get_instruction_format(hdtRegisterOffset) == HDT_RO);
//     REQUIRE(util::get_instruction_format(singleDataTransfer) == SDT);
//     REQUIRE(util::get_instruction_format(blockDataTransfer) == BDT);
//     REQUIRE(util::get_instruction_format(coprocessorDataOperation) == CDO);
//     REQUIRE(util::get_instruction_format(coprocessorDataTransfer) ==  CDT);
//     REQUIRE(util::get_instruction_format(branch) == B);
//     REQUIRE(util::get_instruction_format(dataProcessing) == DP);

//     // some random ones that should be false
//     REQUIRE(util::get_instruction_format(hdtRegisterOffset) != BEX);
//     REQUIRE(util::get_instruction_format(singleDataTransfer) != MUL);
//     REQUIRE(util::get_instruction_format(blockDataTransfer) != UNDEF);
//     REQUIRE(util::get_instruction_format(coprocessorDataOperation) != HDT_RO);
//     REQUIRE(util::get_instruction_format(coprocessorDataTransfer) != DP);
//     REQUIRE(util::get_instruction_format(branch) != BEX);
// }

TEST_CASE("InstructionSubset", "[instruction_subset]") {
    u32 a = 0b00000000000000000000000000000000;
    u32 b = 0b00000000000000000000000000001011;
    u32 c = 0b11001000000000000000000000000000;
    u32 d = 0b11000000000000000000110010011011;

    REQUIRE(util::get_instruction_subset(a, 3, 0) == 0b0000);
    REQUIRE(util::get_instruction_subset(b, 3, 0) == 0b1011);
    REQUIRE(util::get_instruction_subset(c, 31, 28) == 0b1100);
    REQUIRE(util::get_instruction_subset(d, 11, 0) == 0b110010011011);
    REQUIRE(util::get_instruction_subset(c, 27, 27) == 0b1);

    // flipped order of operands
    REQUIRE(util::get_instruction_subset(b, 0, 4) == 0b0000);
}

TEST_CASE("MemoryTests", "[mem_tests]") {
    Memory mem;
    u32 address = 0x1000;
    mem.write_u32(address, 0xABCDEFA0);
    REQUIRE(mem.read_u8(address) == 0xA0);
    REQUIRE(mem.read_u32(address) == 0xABCDEFA0);

    // memory region test
    mem.write_u32(0x4000000, 0xAABBCCDD); // byte 0 of io_reg memory region
    REQUIRE(mem.memory.io_reg[0] == 0xDD);

    // game_rom memory test
    Memory mem2;
    mem2.game_rom = new u8[8]();
    mem2.write_u32(0x8000004, 0xAABBCCDD);
    REQUIRE(mem2.game_rom[4] == 0xDD);
    REQUIRE(mem2.read_u8(0x8000007) == 0xAA);
    std::cout << std::hex << mem2.game_rom[4];
}