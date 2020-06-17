#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <bitset>

#include "../cpu.h"
#include "../common.h"
#include "../instruction.h"

TEST_CASE("ConditionFlagTests", "[condition_flag]") {
    cpu a;
    
    // REQUIRE all flags are 0 upon initialization
    REQUIRE(a.arm.getConditionCodeFlag(N) == 0);
    REQUIRE(a.arm.getConditionCodeFlag(Z) == 0);
    REQUIRE(a.arm.getConditionCodeFlag(C) == 0);
    REQUIRE(a.arm.getConditionCodeFlag(V) == 0);

    // set N, C, and V flags to 1
    a.arm.setConditionCodeFlag(N, 1);
    a.arm.setConditionCodeFlag(C, 1);
    a.arm.setConditionCodeFlag(V, 1);

    // REQUIRE N, C, and V flags are now 1 and Z is still 0
    REQUIRE(a.arm.getConditionCodeFlag(N) == 1);
    REQUIRE(a.arm.getConditionCodeFlag(C) == 1);
    REQUIRE(a.arm.getConditionCodeFlag(V) == 1);
    REQUIRE(a.arm.getConditionCodeFlag(Z) == 0);

    // now set Z flag to 1 and REQUIRE it got changed
    a.arm.setConditionCodeFlag(Z, 1);
    REQUIRE(a.arm.getConditionCodeFlag(Z) == 1);

    // set N back to 0 and REQUIRE it got changed
    a.arm.setConditionCodeFlag(N, 0);
    REQUIRE(a.arm.getConditionCodeFlag(N) == 0);
    
    // try setting the code of a non recognized flag (should print some error message)
    a.arm.setConditionCodeFlag((ConditionCodeFlag_t) 9, 1);
    // try setting the code of a non recognized bit (should print some error message)
    a.arm.setConditionCodeFlag(Z, 4);
}

TEST_CASE("InstructionConditionField", "[instruction_condition_field]") {
    cpu a;
    Instruction i = 0b00000000000000000000000000000000; // 32 bits

    // a zerod cpu should match a zero'd instruction
    REQUIRE(isConditionMet(i, a.arm) == true);

    i = 0b10000000000000000000000000000000; // n flag

    // n flag set with zero'd cpu should REQUIRE false
    REQUIRE(isConditionMet(i, a.arm) == false);

    // set N, C, and V flags to 1
    a.arm.setConditionCodeFlag(N, 1);
    a.arm.setConditionCodeFlag(C, 1);
    a.arm.setConditionCodeFlag(V, 1);
    
    // with cpu condition bits at 1011, should still REQUIRE false with this instruction
    REQUIRE(isConditionMet(i, a.arm) == false);

    i = 0b10110000000000000000000000000000;
    // however, this instruction should be true
    REQUIRE(isConditionMet(i, a.arm) == true);

    // set N, C, and V flags to 0 and Z to 1
    a.arm.setConditionCodeFlag(N, 0);
    a.arm.setConditionCodeFlag(C, 0);
    a.arm.setConditionCodeFlag(V, 0);
    a.arm.setConditionCodeFlag(Z, 1);

    // 0100 should be false with this condition
    REQUIRE(isConditionMet(i, a.arm) == false);

    // but true with this one
    i = 0b01000000000000000000000000000000;
    REQUIRE(isConditionMet(i, a.arm) == true);
}

TEST_CASE("InstructionSetFormat", "[instruction_set_format]") {
    Instruction i = 0;
    
    // zero'd instruction is actually a valid data processing instruction
    REQUIRE(getInstructionFormat(i) == DP);

    // see docs/arm_instruction_set_bitfield.png to see which bits should be set for which instruction formats
    //  (one's digit of instruction) -                10987654321098765432109876543210
    Instruction dataProcessing               =      0b00000000000000000000000000000000;
    Instruction multiply                     =      0b00000000000000000000000010010000;
    Instruction multiplyLong                 =      0b00000000100000000000000010010000;
    Instruction singleDataSwap               =      0b00000001000000000000000010010000;
    Instruction branchExchange               =      0b00000001001011111111111100010000;
    Instruction hdtRegisterOffset            =      0b00000000000000000000000010010000;
    Instruction hdtImmediateOffset           =      0b00000000010000000000000010010000;
    Instruction singleDataTransfer           =      0b00000100000000000000000000000000;
    Instruction undefined                    =      0b00000110000000000000000000010000;
    Instruction blockDataTransfer            =      0b00001000000000000000000000000000;
    Instruction branch                       =      0b00001010000000000000000000000000;
    Instruction coprocessorDataTransfer      =      0b00001100000000000000000000000000;
    Instruction coprocessorDataOperation     =      0b00001110000000000000000000000000;
    Instruction coprocessorRegisterTransfer  =      0b00001110000000000000000000010000;
    Instruction softwareInterrupt            =      0b00001111000000000000000000000000;

    // comparing these hypothetical instructions with their truth,
    // in somewhat random order to make sure order is irrelvant for this function
    REQUIRE(getInstructionFormat(hdtImmediateOffset) == HDT_IO);
    REQUIRE(getInstructionFormat(branchExchange) == BEX);
    REQUIRE(getInstructionFormat(softwareInterrupt) == INT);
    REQUIRE(getInstructionFormat(singleDataSwap) == SDS);
    REQUIRE(getInstructionFormat(multiplyLong) == MULL);
    REQUIRE(getInstructionFormat(multiply) == MUL);
    REQUIRE(getInstructionFormat(coprocessorRegisterTransfer) == CRT);
    REQUIRE(getInstructionFormat(undefined) == UNDEF);
    REQUIRE(getInstructionFormat(hdtRegisterOffset) == HDT_RO);
    REQUIRE(getInstructionFormat(singleDataTransfer) == SDT);
    REQUIRE(getInstructionFormat(blockDataTransfer) == BDT);
    REQUIRE(getInstructionFormat(coprocessorDataOperation) == CDO);
    REQUIRE(getInstructionFormat(coprocessorDataTransfer) ==  CDT);
    REQUIRE(getInstructionFormat(branch) == B);
    REQUIRE(getInstructionFormat(dataProcessing) == DP);

    // some random ones that should be false
    REQUIRE(getInstructionFormat(hdtRegisterOffset) != BEX);
    REQUIRE(getInstructionFormat(singleDataTransfer) != MUL);
    REQUIRE(getInstructionFormat(blockDataTransfer) != UNDEF);
    REQUIRE(getInstructionFormat(coprocessorDataOperation) != HDT_RO);
    REQUIRE(getInstructionFormat(coprocessorDataTransfer) != DP);
    REQUIRE(getInstructionFormat(branch) != BEX);
}

TEST_CASE("InstructionSubset", "[instruction_subset]") {
    Instruction a = 0b00000000000000000000000000000000;
    Instruction b = 0b00000000000000000000000000001011;
    Instruction c = 0b11001000000000000000000000000000;
    Instruction d = 0b11000000000000000000110010011011;

    REQUIRE(getInstructionSubset(a, 3, 0) == 0b0000);
    REQUIRE(getInstructionSubset(b, 3, 0) == 0b1011);
    REQUIRE(getInstructionSubset(c, 31, 28) == 0b1100);
    REQUIRE(getInstructionSubset(d, 11, 0) == 0b110010011011);
    REQUIRE(getInstructionSubset(c, 27, 27) == 0b1);

    // flipped order of operands
    REQUIRE(getInstructionSubset(b, 0, 4) == 0b0000);
}