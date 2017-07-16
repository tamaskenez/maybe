#include <numeric>

#include "std.h"
#include "nowide/args.hpp"

#include "fmt/format.h"
#include "ul/ul.h"

#include "command_line.h"
#include "consts.h"
#include "compiler.h"
#include "log.h"

namespace maybe {

static const char* const c_usage_text =
    R"~~~~({0} compiler

Usage: {0} --help
       {0} <input-files>
)~~~~";

void compiler_test()
{
    // test vector::assign
    vector<int> v(16);
    auto C = v.capacity();
    v.resize(C);
    std::iota(BE(v), 0);
    auto d = v.data();
    int N = v.size() * 3 / 4;
    auto i0 = v.end()[-N];
    // copy assign an overlapping range of last items to the same vector
    v.assign(v.end() - N, v.end());
    v.resize(C);
    // check size and that no reallocation took place
    bool ok = v.size() == C && v.data() == d;
    // check values;
    for (int i = 0; i < N; ++i)
        ok = ok && v[i] == i0 + i;
    if (!ok)
        log_fatal(
            "internal error, compiler test failed (std::vector does not work "
            "as intended, fix FileReader).");
}

int main(int argc, char* argv[])
{
    try {
        nowide::args nwa(argc, argv);  // converts args to utf8 (windows-only)
        compiler_test();
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
