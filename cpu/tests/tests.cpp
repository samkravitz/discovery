#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <bitset>

#include "../cpu.h"
#include "../common.h"
#include "../util.h"

TEST_CASE("ConditionFlagTests", "[condition_flag]") {
    cpu a;
    
    // REQUIRE all flags are 0 upon initialization
    REQUIRE(a.arm.get_condition_code_flag(N) == 0);
    REQUIRE(a.arm.get_condition_code_flag(Z) == 0);
    REQUIRE(a.arm.get_condition_code_flag(C) == 0);
    REQUIRE(a.arm.get_condition_code_flag(V) == 0);

    // set N, C, and V flags to 1
    a.arm.set_condition_code_flag(N, 1);
    a.arm.set_condition_code_flag(C, 1);
    a.arm.set_condition_code_flag(V, 1);

    // REQUIRE N, C, and V flags are now 1 and Z is still 0
    REQUIRE(a.arm.get_condition_code_flag(N) == 1);
    REQUIRE(a.arm.get_condition_code_flag(C) == 1);
    REQUIRE(a.arm.get_condition_code_flag(V) == 1);
    REQUIRE(a.arm.get_condition_code_flag(Z) == 0);

    // now set Z flag to 1 and REQUIRE it got changed
    a.arm.set_condition_code_flag(Z, 1);
    REQUIRE(a.arm.get_condition_code_flag(Z) == 1);

    // set N back to 0 and REQUIRE it got changed
    a.arm.set_condition_code_flag(N, 0);
    REQUIRE(a.arm.get_condition_code_flag(N) == 0);
    
    // try setting the code of a non recognized flag (should print some error message)
    a.arm.set_condition_code_flag((condition_code_flag_t) 9, 1);
    // try setting the code of a non recognized bit (should print some error message)
    a.arm.set_condition_code_flag(Z, 4);
}

TEST_CASE("InstructionConditionField", "[instruction_condition_field]") {
    cpu a;
    arm_instruction i = 0b00000000000000000000000000000000; // 32 bits

    // a zerod cpu should match a zero'd arm_instruction
    REQUIRE(util::condition_met(i, a.arm) == true);

    i = 0b10000000000000000000000000000000; // n flag

    // n flag set with zero'd cpu should REQUIRE false
    REQUIRE(util::condition_met(i, a.arm) == false);

    // set N, C, and V flags to 1
    a.arm.set_condition_code_flag(N, 1);
    a.arm.set_condition_code_flag(C, 1);
    a.arm.set_condition_code_flag(V, 1);
    
    // with cpu condition bits at 1011, should still REQUIRE false with this instruction
    REQUIRE(util::condition_met(i, a.arm) == false);

    i = 0b10110000000000000000000000000000;
    // however, this instruction should be true
    REQUIRE(util::condition_met(i, a.arm) == true);

    // set N, C, and V flags to 0 and Z to 1
    a.arm.set_condition_code_flag(N, 0);
    a.arm.set_condition_code_flag(C, 0);
    a.arm.set_condition_code_flag(V, 0);
    a.arm.set_condition_code_flag(Z, 1);

    // 0100 should be false with this condition
    REQUIRE(util::condition_met(i, a.arm) == false);

    // but true with this one
    i = 0b01000000000000000000000000000000;
    REQUIRE(util::condition_met(i, a.arm) == true);
}

TEST_CASE("InstructionSetFormat", "[instruction_set_format]") {
    arm_instruction i = 0;
    
    // zero'd instruction is actually a valid data processing instruction
    REQUIRE(util::get_instruction_format(i) == DP);

    // see docs/arm_instruction_set_bitfield.png to see which bits should be set for which instruction formats
    //  (one's digit of instruction) -                10987654321098765432109876543210
    arm_instruction dataProcessing               =      0b00000000000000000000000000000000;
    arm_instruction multiply                     =      0b00000000000000000000000010010000;
    arm_instruction multiplyLong                 =      0b00000000100000000000000010010000;
    arm_instruction singleDataSwap               =      0b00000001000000000000000010010000;
    arm_instruction branchExchange               =      0b00000001001011111111111100010000;
    arm_instruction hdtRegisterOffset            =      0b00000000000000000000000010010000;
    arm_instruction hdtImmediateOffset           =      0b00000000010000000000000010010000;
    arm_instruction singleDataTransfer           =      0b00000100000000000000000000000000;
    arm_instruction undefined                    =      0b00000110000000000000000000010000;
    arm_instruction blockDataTransfer            =      0b00001000000000000000000000000000;
    arm_instruction branch                       =      0b00001010000000000000000000000000;
    arm_instruction coprocessorDataTransfer      =      0b00001100000000000000000000000000;
    arm_instruction coprocessorDataOperation     =      0b00001110000000000000000000000000;
    arm_instruction coprocessorRegisterTransfer  =      0b00001110000000000000000000010000;
    arm_instruction softwareInterrupt            =      0b00001111000000000000000000000000;

    // comparing these hypothetical instructions with their truth,
    // in somewhat random order to make sure order is irrelvant for this function
    REQUIRE(util::get_instruction_format(hdtImmediateOffset) == HDT_IO);
    REQUIRE(util::get_instruction_format(branchExchange) == BEX);
    REQUIRE(util::get_instruction_format(softwareInterrupt) == INT);
    REQUIRE(util::get_instruction_format(singleDataSwap) == SDS);
    REQUIRE(util::get_instruction_format(multiplyLong) == MULL);
    REQUIRE(util::get_instruction_format(multiply) == MUL);
    REQUIRE(util::get_instruction_format(coprocessorRegisterTransfer) == CRT);
    REQUIRE(util::get_instruction_format(undefined) == UNDEF);
    REQUIRE(util::get_instruction_format(hdtRegisterOffset) == HDT_RO);
    REQUIRE(util::get_instruction_format(singleDataTransfer) == SDT);
    REQUIRE(util::get_instruction_format(blockDataTransfer) == BDT);
    REQUIRE(util::get_instruction_format(coprocessorDataOperation) == CDO);
    REQUIRE(util::get_instruction_format(coprocessorDataTransfer) ==  CDT);
    REQUIRE(util::get_instruction_format(branch) == B);
    REQUIRE(util::get_instruction_format(dataProcessing) == DP);

    // some random ones that should be false
    REQUIRE(util::get_instruction_format(hdtRegisterOffset) != BEX);
    REQUIRE(util::get_instruction_format(singleDataTransfer) != MUL);
    REQUIRE(util::get_instruction_format(blockDataTransfer) != UNDEF);
    REQUIRE(util::get_instruction_format(coprocessorDataOperation) != HDT_RO);
    REQUIRE(util::get_instruction_format(coprocessorDataTransfer) != DP);
    REQUIRE(util::get_instruction_format(branch) != BEX);
}

TEST_CASE("InstructionSubset", "[instruction_subset]") {
    arm_instruction a = 0b00000000000000000000000000000000;
    arm_instruction b = 0b00000000000000000000000000001011;
    arm_instruction c = 0b11001000000000000000000000000000;
    arm_instruction d = 0b11000000000000000000110010011011;

    REQUIRE(util::get_instruction_subset(a, 3, 0) == 0b0000);
    REQUIRE(util::get_instruction_subset(b, 3, 0) == 0b1011);
    REQUIRE(util::get_instruction_subset(c, 31, 28) == 0b1100);
    REQUIRE(util::get_instruction_subset(d, 11, 0) == 0b110010011011);
    REQUIRE(util::get_instruction_subset(c, 27, 27) == 0b1);

    // flipped order of operands
    REQUIRE(util::get_instruction_subset(b, 0, 4) == 0b0000);
}