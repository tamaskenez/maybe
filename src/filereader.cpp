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
}
