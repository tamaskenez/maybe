#include "nowide/args.hpp"
#include "nowide/cstdio.hpp"
#include "nowide/cstdlib.hpp"

#include "ul/ul.h"

#include "command_line.h"

namespace maybe {
int main(int argc, char* argv[])
{
    try {
        nowide::args nwa(argc, argv);  // converts args to utf8 (windows-only)
        auto cl = parse_command_line(argc, argv);
        return EXIT_SUCCESS;
    } catch (std::exception& e) {
        fprintf(stderr, "Aborting, exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "Aborting, unknown exception\n");
    }
    return EXIT_FAILURE;
}
}

int main(int argc, char* argv[])
{
    return maybe::main(argc, argv);
}
