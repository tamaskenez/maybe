#include "compiler.h"

#include "log.h"
#include "filereader.h"

namespace maybe {

void compile_file(string_par f)
{
    auto lr = FileReader::new_(f.c_str());
    if (is_left(lr)) {
        log_fatal(left(lr), "can't open file '{}'", f.c_str());
    }
    auto fr = move(right(lr));
}

void run_compiler(const CommandLine& cl)
{
    for (auto& f : cl.files)
        compile_file(f);
}
}
