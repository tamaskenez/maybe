#pragma once

#include "std.h"

namespace maybe {

template <class X, class Y>
using Either = variant<X, Y>;

template <class X, class Y>
bool is_left(const Either<X, Y>& v)
{
    return v.index() == 0;
}

template <class X, class Y>
bool is_right(const Either<X, Y>& v)
{
    return v.index() == 0;
}

template <class X, class Y>
const X& left(const Either<X, Y>& v)
{
    return mpark::get<0>(v);
}

template <class X, class Y>
const Y& right(const Either<X, Y>& v)
{
    return mpark::get<1>(v);
}

template <class X, class Y>
X& left(Either<X, Y>& v)
{
    return mpark::get<0>(v);
}

template <class X, class Y>
Y& right(Either<X, Y>& v)
{
    return mpark::get<1>(v);
}
struct ErrorAccu
{
    void operator+=(const ErrorAccu& x) { num_errors += x.num_errors; }
    int num_errors;
};
struct ErrorInSourceFile
{
    bool has_location() const { return line_num > 0 && col > 0; }

    string filename;
    string msg;
    int line_num = 0;  // 1-based
    int col = 0;       // 1-based
    int length = 0;
};
void report_error(const ErrorInSourceFile& x);
}
