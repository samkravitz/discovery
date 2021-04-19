/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 *
 * FILE: log.h
 * DATE: January 6th, 2021
 * DESCRIPTION: specialized logging for discovery using the fmt library
 */

#pragma once

#include "common.h"

#include <fmt/format.h>
#include <fmt/color.h>

enum class LogLevel
{
    Message, // white
    Debug,   // green
    Warning, // yellow
    Error    // red
};

template<typename S, typename... Args>
void LOG(S const &format, Args&&... args)
{
    LOG(LogLevel::Message, format, args...);
}

template<typename S, typename... Args>
void LOG(LogLevel level, S const &format, Args&&... args)
{
    fmt::color color;

    switch (level)
    {
        case LogLevel::Message: color = fmt::color::white;          break;
        case LogLevel::Debug:   color = fmt::color::dark_sea_green; break;
        case LogLevel::Warning: color = fmt::color::gold;           break;
        case LogLevel::Error:   color = fmt::color::fire_brick;     break;
    }

    fmt::print(fg(color), format, args...);
}
