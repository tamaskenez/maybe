#include "std.h"

#include "nowide/args.hpp"

#include "fmt/format.h"
#include "ul/ul.h"

#include "command_line.h"
#include "consts.h"
#include "compiler.h"

namespace maybe {

static const char* const c_usage_text =
    R"~~~~({0} compiler

Usage: {0} --help
       {0} <input-files>
)~~~~";

int main(int argc, char* argv[])
{
    try {
        nowide::args nwa(argc, argv);  // converts args to utf8 (windows-only)
        auto cl = parse_command_line(argc, argv);
        if (cl.help)
            fmt::print(c_usage_text, c_program_name);
        else
            run_compiler(cl);
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
