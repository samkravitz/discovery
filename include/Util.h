/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: util.h
 * DATE: June 19, 2020
 * DESCRIPTION: utility functions that don't belong in any class
 */

#pragma once

#include <bitset>
#include <fmt/format.h>

#include "common.h"

namespace Util
{
    // determine which type of operation the instruction is
    ArmInstruction GetInstructionFormat(u32 instruction);

    // determine which type of thumb operation an instruction is
    ThumbInstruction GetInstructionFormat(u16 instruction);
    

    /* get subset of bits for purposes like destination register, opcode, shifts
    * All instructions have data hidden within their codes;
    * ex: A branch instruction holds the offset in bits [23-0]
    * This function will extract the bits.
    * Since the reference I am using is in reverse bit order, end >= start must be true
    * 
    * ex: bitseq<7, 4>(0b11110000) = 0b1111
    */
    template <int end, int start>
    u32 bitseq(u32 val)
    {
        if (end < start)
            return 0;

        std::bitset<32> bits(val);
        u32 subset = 0;

        for (int i = end; i >= start; --i)
        {
            subset <<= 1;
            subset |= bits[i];
        }

        return subset;
    }

    template <int end, int start>
    u16 bitseq(u16 val)
    {
        if (end < start)
            return 0;

        std::bitset<16> bits(val);
        u16 subset = 0;

        for (int i = end; i >= start; --i)
        {
            subset <<= 1;
            subset |= bits[i];
        }

        return subset;
    }

    enum class LogLevel
    {
        Message, // white
        Debug,   // green
        Warning, // yellow
        Error    // red
    };

    template<typename S, typename... Args>
    void LOG(const S& format, Args&&... args)
    {
        LOG(LogLevel::Message, format, args...);
    }

    template<typename S, typename... Args>
    void LOG(LogLevel level, const S& format, Args&&... args)
    {
        std::string color;

        switch (level)
        {
            case LogLevel::Message: color = "\e[0;37m"; break;
            case LogLevel::Debug:   color = "\e[0;32m"; break;
            case LogLevel::Warning: color = "\e[0;93m"; break;
            case LogLevel::Error:   color = "\e[0;31m"; break;
        }

        fmt::print("{}", color);
        fmt::vprint(format, fmt::make_args_checked<Args...>(format, args...));
    }
}

// LOG(Util::LogLevel::Message, "");
// LOG(Util::LogLevel::Debug, "");
// LOG(Util::LogLevel::Warning, "");
// LOG(Util::LogLevel::Error, "");