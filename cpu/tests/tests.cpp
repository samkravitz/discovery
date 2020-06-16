#include <assert.h>
#include <iostream>
#include <bitset>

#include "../cpu.h"
#include "../common.h"
#include "../instruction.h"

// assert macro with additional debugging info courtesy of
// https://stackoverflow.com/questions/2193544/how-to-print-additional-information-when-assert-fails
#define ASSERT(left,operator,right) { if(!((left) operator (right))){ std::cerr << "ASSERT FAILED: " << #left << #operator << #right << " @ " << __FILE__ << " (" << __LINE__ << "). " << #left << "=" << (left) << "; " << #right << "=" << (right) << std::endl; } }

void conditionFlagTests() {
    cpu a;
    
    // assert all flags are 0 upon initialization
    assert(a.a.getConditionCodeFlag(N) == 0);
    assert(a.a.getConditionCodeFlag(Z) == 0);
    assert(a.a.getConditionCodeFlag(C) == 0);
    assert(a.a.getConditionCodeFlag(V) == 0);

    // set N, C, and V flags to 1
    a.a.setConditionCodeFlag(N, 1);
    a.a.setConditionCodeFlag(C, 1);
    a.a.setConditionCodeFlag(V, 1);

    // assert N, C, and V flags are now 1 and Z is still 0
    assert(a.a.getConditionCodeFlag(N) == 1);
    assert(a.a.getConditionCodeFlag(C) == 1);
    assert(a.a.getConditionCodeFlag(V) == 1);
    assert(a.a.getConditionCodeFlag(Z) == 0);

    // now set Z flag to 1 and assert it got changed
    a.a.setConditionCodeFlag(Z, 1);
    assert(a.a.getConditionCodeFlag(Z) == 1);

    // set N back to 0 and assert it got changed
    a.a.setConditionCodeFlag(N, 0);
    assert(a.a.getConditionCodeFlag(N) == 0);
    
    // try setting the code of a non recognized flag (should print some error message)
    a.a.setConditionCodeFlag((ConditionCodeFlag_t) 9, 1);
    // try setting the code of a non recognized bit (should print some error message)
    a.a.setConditionCodeFlag(Z, 4);
}

void instructionConditionFieldTests() {
    cpu a;
    Instruction i = 0b00000000000000000000000000000000; // 32 bits

    // a zerod cpu should match a zero'd instruction
    assert(isConditionMet(i, a.a) == true);

    i = 0b10000000000000000000000000000000; // n flag

    // n flag set with zero'd cpu should assert false
    assert(isConditionMet(i, a.a) == false);

    // set N, C, and V flags to 1
    a.a.setConditionCodeFlag(N, 1);
    a.a.setConditionCodeFlag(C, 1);
    a.a.setConditionCodeFlag(V, 1);
    
    // with cpu condition bits at 1011, should still assert false with this instruction
    assert(isConditionMet(i, a.a) == false);

    i = 0b10110000000000000000000000000000;
    // however, this instruction should be true
    assert(isConditionMet(i, a.a) == true);

    // set N, C, and V flags to 0 and Z to 1
    a.a.setConditionCodeFlag(N, 0);
    a.a.setConditionCodeFlag(C, 0);
    a.a.setConditionCodeFlag(V, 0);
    a.a.setConditionCodeFlag(Z, 1);

    // 0100 should be false with this condition
    assert(isConditionMet(i, a.a) == false);

    // but true with this one
    i = 0b01000000000000000000000000000000;
    assert(isConditionMet(i, a.a) == true);
}

void instructionSetFormatTests() {
    Instruction i = 0;
    
    // zero'd instruction is actually a valid data processing instruction
    assert(getInstructionFormat(i) == DP);

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
    ASSERT(getInstructionFormat(hdtImmediateOffset), ==, HDT_IO);
    ASSERT(getInstructionFormat(branchExchange), ==,BEX);
    ASSERT(getInstructionFormat(softwareInterrupt), ==, INT);
    ASSERT(getInstructionFormat(singleDataSwap), ==, SDS);
    ASSERT(getInstructionFormat(multiplyLong), ==, MULL);
    ASSERT(getInstructionFormat(multiply), ==, MUL);
    ASSERT(getInstructionFormat(coprocessorRegisterTransfer), ==, CRT);
    ASSERT(getInstructionFormat(undefined), ==, UNDEF);
    ASSERT(getInstructionFormat(hdtRegisterOffset), ==, HDT_RO);
    ASSERT(getInstructionFormat(singleDataTransfer), ==, SDT);
    ASSERT(getInstructionFormat(blockDataTransfer), ==, BDT);
    ASSERT(getInstructionFormat(coprocessorDataOperation), ==, CDO);
    ASSERT(getInstructionFormat(coprocessorDataTransfer), ==, CDT);
    ASSERT(getInstructionFormat(branch), ==, B);
    ASSERT(getInstructionFormat(dataProcessing), ==, DP);

    // some random ones that should be false
    assert(getInstructionFormat(hdtRegisterOffset) != BEX);
    assert(getInstructionFormat(singleDataTransfer) != MUL);
    assert(getInstructionFormat(blockDataTransfer) != UNDEF);
    assert(getInstructionFormat(coprocessorDataOperation) != HDT_RO);
    assert(getInstructionFormat(coprocessorDataTransfer) != DP);
    assert(getInstructionFormat(branch) != BEX);
}

int main() {
    conditionFlagTests();
    instructionConditionFieldTests();
    instructionSetFormatTests();
    return 0;
}

