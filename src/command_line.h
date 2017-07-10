#pragma once

#include "std.h"

namespace maybe {
struct CommandLine
{
    bool help = false;
    vector<string> files;
    string out;
};

using ize = char const* const;
CommandLine parse_command_line(int argc, const char* const argv[]);
}
