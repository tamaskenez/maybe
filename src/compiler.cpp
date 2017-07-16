#include "compiler.h"

#include "log.h"
#include "filereader.h"

namespace maybe {

void compile_line(cspan filename, int line_num, cspan line)
{
    LOG_DEBUG("[{}:{}] <<{}>>", filename, line_num, line);
}

void compile_file(string_par f)
{
    auto filename = ul::make_span(f.c_str());
    auto lr = FileReader::new_(f.c_str());
    if (is_left(lr)) {
        log_fatal(left(lr), "can't open file '{}'", f.c_str());
    }
    auto fr = move(right(lr));
    for (;;) {
        auto maybe_line = fr.read_next_line();
        if (maybe_line)
            compile_line(filename, fr.line_num(), *maybe_line);
        else
            break;
    }
}

void run_compiler(const CommandLine& cl)
{
    for (auto& f : cl.files)
        compile_file(f);
}
}
