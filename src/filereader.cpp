#include "filereader.h"
#include "log.h"
#include "ul/check.h"

namespace maybe {

Either<system_error, FileReader> FileReader::new_(string filename)
{
    FILE* f = nowide::fopen(filename.c_str(), "rb");
    if (f)
        return FileReader(f, move(filename));
    else
        return system_error(errno, system_category());
}

FileReader::~FileReader()
{
    if (f) {
        int r = fclose(f);
        if (r != 0)
            LOG_DEBUG("fclose(\"{}\") -> {}", filename, r);
    }
}

FileReader::FileReader(FILE* f, string filename)
    : read_buf(new ReadBuf), f(f), filename(move(filename))
{
    p.next_char_to_read = p.read_buf_end = &read_buf->front();
}

int FileReader::read_ahead_at_least(int n)
{
    CHECK(n <= c_filereader_read_buf_capacity);
    const auto bytes_in_buf = p.read_buf_end - p.next_char_to_read;
    if (bytes_in_buf >= n)
        return bytes_in_buf;
    if (bytes_in_buf > 0 && p.next_char_to_read > &read_buf->front()) {
        // move unread slice of read_buf down to &read_buf->front()
        std::copy(p.next_char_to_read, p.read_buf_end, &read_buf->front());
        p.next_char_to_read -= &read_buf->front();
        p.read_buf_end -= p.next_char_to_read - &read_buf->front();
    }
    auto bytes_read = fread((void*)p.read_buf_end, 1,
                            c_filereader_read_buf_capacity - bytes_in_buf, f);
    p.read_buf_end += bytes_read;
    return p.read_buf_end - p.next_char_to_read;
}

Maybe<char> FileReader::peek_char_in_read_buf(int i)
{
    if (p.read_buf_end - p.next_char_to_read >= i)
        return (p.next_char_to_read)[i];
    else
        return Nothing;
}
}
