#include "compiler.h"

#include <system_error>

#include "fmt/format.h"

#include "std.h"
#include "log.h"

namespace maybe {

using std::system_error;
using std::system_category;

template <class X, class Y>
using Either = std::variant<X, Y>;

class FileReader
{
public:
    static Either<system_error, FileReader> create(string filename)
    {
        FILE* f = fopen(filename.c_str(), "rb");
        if (f)
            return FileReader(f, move(filename));
        else
            return system_error(errno, system_category());
    }
    FileReader(const FileReader&) = delete;
    FileReader(FileReader&& x) : f(x.f), filename(move(x.filename))
    {
        x.f = nullptr;
    }
    void operator=(const FileReader&) = delete;
    void operator=(FileReader&&) = delete;

    ~FileReader()
    {
        if (f) {
            int r = fclose(f);
            if (r != 0)
                log_debug("fclose(\"{}\") -> {}", filename, r);
        }
    }

private:
    explicit FileReader(FILE* f, string filename)
        : f(f), filename(move(filename))
    {
    }

    FILE* f = nullptr;
    string filename;
};

void compile_file(string_par f)
{
    auto lr = FileReader::create(f);
    if (is_left(lr)) {
        log_fatal(left(lr), "can't open '{}'", f.c_str());
    }
    auto fr = move(right(lr));
}

void run_compiler(const CommandLine& cl)
{
    for (auto& f : cl.files)
        compile_file(f);
}
}
