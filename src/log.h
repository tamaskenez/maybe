#pragma once

#include <cstdio>
#include <cstdlib>

#include "fmt/format.h"

#include "consts.h"

namespace maybe {
template <typename... Args>
[[noreturn]] void log_fatal(const char* format, const Args&... args)
{
    fprintf(stderr, "%s: error: ", c_program_name);
    fmt::print(stderr, format, args...);
    fprintf(stderr, "\n");
    std::exit(EXIT_FAILURE);
}
}
