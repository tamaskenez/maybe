#pragma once

#include "std.h"
#include "utils.h"
#include "consts.h"

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

    Maybe<cspan> read_next_line();  // false if no more lines
    int line_num() const { return line_num_; }

private:
    FileReader(FILE* f, string filename);

    FILE* f = nullptr;
    string filename;
    int line_num_ = 0;
    vector<char> read_buf;
    int next_unprocessed_idx = 0;
};
}
