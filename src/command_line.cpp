#include <cassert>

#include "ul/string.h"
#include "ul/ul.h"
#include "ul/string_par.h"

#include "log.h"
#include "command_line.h"

namespace maybe {

using ul::startswith;
using ul::string_par;

CommandLine parse_command_line(int argc, const char* const argv[])
{
    CommandLine cl;
    FOR(i, 1, argc)
    {
        auto a = argv[i];
        if (startswith(a, "--")) {
            a += 2;
            if (startswith(a, "help"))
                cl.help = true;
            else
                log_fatal("invalid option: '{}'", argv[i]);
        } else {
            cl.files.emplace_back(a);
        }
    }
    return cl;
}
}
