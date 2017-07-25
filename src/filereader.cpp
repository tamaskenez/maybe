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
    p.next_char_to_read = p.next_char_to_read = &read_buf->front();
}
}
