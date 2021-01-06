/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: log.h
 * DATE: January 6th, 2021
 * DESCRIPTION: specialized logging for discovery using the fmt library
 */

#pragma once

#include <fmt/format.h>

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
