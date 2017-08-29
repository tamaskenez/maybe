#include "utils.h"

namespace maybe {
void report_error(const ErrorInSourceFile& x)
{
    CHECK(!x.msg.empty() && !x.filename.empty());
    if (x.has_location())
        fmt::print(stderr, "{}:{}:{}: error: {}\n", x.filename, x.line_num,
                   x.col, x.msg);
    else
        fmt::print(stderr, "{}: error: {}\n", x.filename, x.msg);
}
}
