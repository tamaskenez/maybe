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

cspan FileReader::read(int keep_tail_bytes, int read_bytes)
{
    // can't keep more than current chars, can't read more then capacity
    CHECK(keep_tail_bytes <= read_buf_size &&
          (keep_tail_bytes + read_bytes <= read_buf->size()));

    // copy tail to begin
    std::copy(read_buf->end() - keep_tail_bytes, read_buf->end(),
              read_buf->begin());

    auto read_start_ptr = &(read_buf->at(keep_tail_bytes));
    auto bytes_read = fread(read_start_ptr, 1, read_bytes, f);
    read_buf_size = keep_tail_bytes + bytes_read;
    return cspan{&(*read_buf)[0], (size_t)read_buf_size};
}

bool FileReader::is_eof() const
{
    return feof(f);
}

FileReader::FileReader(FILE* f, string filename)
    : f(f), filename(move(filename)), read_buf(new ReadBuf)
{
}
}
