#pragma once

#include <cstdlib>

#include "std.h"

#include "fmt/format.h"

#include "consts.h"
#include "globals.h"

namespace maybe {

template <typename... Args>
[[noreturn]] void log_fatal(const char* format, const Args&... args)
{
    fprintf(stderr, "%s: error: ", c_program_name);
    fmt::print(stderr, format, args...);
    fprintf(stderr, "\n");
    std::exit(EXIT_FAILURE);
}

template <typename... Args>
void report_error(const char* format, const Args&... args)
{
    fprintf(stderr, "%s: error: ", c_program_name);
    fmt::print(stderr, format, args...);
    fprintf(stderr, "\n");
}

template <typename... Args>
[[noreturn]] void log_fatal(const system_error& se,
                            const char* format,
                            const Args&... args)
{
    fmt::print(stderr, "{}: error: {} ({})\n", c_program_name,
               fmt::format(format, args...), se.what());
    std::exit(EXIT_FAILURE);
}

template <typename... Args>
void report_error(const system_error& se,
                  const char* format,
                  const Args&... args)
{
    fmt::print(stderr, "{}: error: {} ({})\n", c_program_name,
               fmt::format(format, args...), se.what());
}

#define LOG_DEBUG(format, ...)                                             \
    (LogLevel::debug <= globals.log_level ? log_debug(format, __VA_ARGS__) \
                                          : (void)0)

template <typename... Args>
void log_debug(const char* format, const Args&... args)
{
    fmt::print(stderr, "{}: debug: {}\n", c_program_name,
               fmt::format(format, args...));
}
}
