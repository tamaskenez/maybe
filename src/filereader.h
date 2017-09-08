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
        : p(x.p), read_buf(move(x.read_buf)), f(x.f), filename(move(x.filename))
    {
        x.p.clear();
        x.f = nullptr;
    }
    void operator=(const FileReader&) = delete;
    void operator=(FileReader&&) = delete;

    ~FileReader();

    bool is_eof() const { return feof(f); }

    // return number of unread bytes in read buf
    int read_ahead_at_least(int n);

    // Return i-th unread char in read_buf (without advancing read position),
    // or Nothing if i is too big.
    // NOTE: Does not fill refill buffer!
    Maybe<char> peek_char_in_read_buf(int i);

    // Advance read pointer and return true if string slice (p, size) is a
    // prefix of current read_buf.
    // NOTE: Does not fill refill buffer!
    bool advance_if_prefix(const char* p, int size);

    // Refills read_buf if empty then return next char without advancing read
    // position.
    // Return Nothing if eof or error
    Maybe<char> peek_next_char()
    {
        if (UL_UNLIKELY(p.next_char_to_read >= p.read_buf_end)) {
            p.next_char_to_read = &read_buf->front();
            auto bytes_read = fread((void*)p.next_char_to_read, 1,
                                    c_filereader_read_buf_capacity, f);
            p.read_buf_end = p.next_char_to_read + bytes_read;
            if (bytes_read == 0)
                return Nothing;
        }
        return *p.next_char_to_read;
    }

    // Refills read_buf if empty then return next char, advances read position.
    // Return Nothing if eof or error
    Maybe<char> next_char()
    {
        if (UL_UNLIKELY(p.next_char_to_read >= p.read_buf_end)) {
            p.next_char_to_read = &read_buf->front();
            auto bytes_read = fread((void*)p.next_char_to_read, 1,
                                    c_filereader_read_buf_capacity, f);
            p.read_buf_end = p.next_char_to_read + bytes_read;
            if (bytes_read == 0)
                return Nothing;
        }
        char c = *p.next_char_to_read;
        ++p.next_char_to_read;
        ++p.chars_read;
        return c;
    }
    void advance()
    {
        assert(p.next_char_to_read < p.read_buf_end);
        ++p.next_char_to_read;
        ++p.chars_read;
    }
    int chars_read() const { return p.chars_read; }

private:
    using ReadBuf = array<char, c_filereader_read_buf_capacity>;

    FileReader(FILE* f, string filename);

    struct P
    {
        void clear()
        {
            next_char_to_read = read_buf_end = nullptr;
            chars_read = 0;
        }

        const char* next_char_to_read = nullptr;
        const char* read_buf_end = nullptr;
        int chars_read = 0;
    } p;

    unique_ptr<ReadBuf> read_buf;
    FILE* f = nullptr;
    string filename;
};
}
