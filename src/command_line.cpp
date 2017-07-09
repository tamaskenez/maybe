#include <cassert>

#include "command_line.h"
#include "ul/string.h"
#include "ul/ul.h"

namespace maybe {

using ul::startswith;

CommandLine parse_command_line(int argc, const char * const argv[])
{
    CommandLine cl;
    FOR(i, 1, argc)
    {
        auto a = argv[i];
        if (startswith(a, "--")) {
            a += 2;
            if(startswith(a, "help"))
            cl.help = true;
            else {
                assert(false);
            }
        } else {
            cl.files.emplace_back(a);
        }
    }
    return cl;
}
}
