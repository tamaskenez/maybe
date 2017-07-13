#pragma once

#include "std.h"
#include "utils.h"

namespace maybe {

class FileReader
{
public:
    static Either<system_error, FileReader> new_(string filename);

    // fow now, only move ctor allowed (add move assignment if needed)
    FileReader(const FileReader&) = delete;
    FileReader(FileReader&& x) : f(x.f), filename(move(x.filename))
    {
        x.f = nullptr;
    }
    void operator=(const FileReader&) = delete;
    void operator=(FileReader&&) = delete;

    ~FileReader();

private:
    FileReader(FILE* f, string filename) : f(f), filename(move(filename)) {}

    FILE* f = nullptr;
    string filename;
};
}
