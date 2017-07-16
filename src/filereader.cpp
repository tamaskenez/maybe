#include "filereader.h"
#include "log.h"

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

Maybe<cspan> FileReader::read_next_line()
{
    int first_idx = next_unprocessed_idx;
    for (;;) {
        // ouf no more chars to process
        if (next_unprocessed_idx >= read_buf.size()) {
            // and we may get some from the file
            if (!feof(f)) {
                if (next_unprocessed_idx == first_idx) {
                    first_idx = next_unprocessed_idx = 0;
                } else if (first_idx != 0) {
                    // move part of line already read to the start of the buf
                    read_buf.assign(read_buf.begin() + first_idx,
                                    read_buf.end());
                    next_unprocessed_idx -= first_idx;
                    first_idx = 0;
                }
                read_buf.resize(next_unprocessed_idx + c_read_buf_min_reserve);
                auto bytes_to_read = read_buf.size() - next_unprocessed_idx;
                auto bytes_read = fread((void*)&read_buf[next_unprocessed_idx],
                                        1, bytes_to_read, f);
                if (bytes_read < bytes_to_read && !feof(f))
                    log_fatal("error reading '{}'", filename);
                read_buf.resize(next_unprocessed_idx + bytes_read);
            }
            // can't read more characters
            if (next_unprocessed_idx >= read_buf.size()) {
                if (next_unprocessed_idx > first_idx) {
                    ++line_num_;
                    return cspan(&read_buf[first_idx],
                                 next_unprocessed_idx - first_idx);
                } else
                    return Nothing;
            }
        }

        for (int i = next_unprocessed_idx; i < read_buf.size(); ++i) {
            if (read_buf[i] == c_ascii_LF) {
                next_unprocessed_idx = i + 1;
                ++line_num_;
                return cspan(read_buf.data() + first_idx, i - first_idx);
            }
        }
        next_unprocessed_idx = read_buf.size();
        // out of chars and no EOL
    }
}
FileReader::FileReader(FILE* f, string filename)
    : f(f), filename(move(filename))
{
    read_buf.reserve(c_read_buf_default_reserve);
}
}
