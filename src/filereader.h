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
    FileReader(FileReader&& x)
        : f(x.f),
          filename(move(x.filename)),
          read_buf(move(x.read_buf)),
          read_buf_size(x.read_buf_size)
    {
        x.f = nullptr;
        x.read_buf_size = 0;
    }
    void operator=(const FileReader&) = delete;
    void operator=(FileReader&&) = delete;

    ~FileReader();

    // keep the last `keep_tail_bytes` from the end of the previous buffer
    // and read `read_bytes` new bytes
    // may return smaller buffer if eof or error
    cspan read(int keep_tail_bytes, int read_bytes);
    bool is_eof() const;

private:
    FileReader(FILE* f, string filename);

    FILE* f = nullptr;
    string filename;
    using ReadBuf = array<char, c_filereader_read_buf_capacity>;
    unique_ptr<ReadBuf> read_buf;
    int read_buf_size = 0;  // actual characters
};
}
