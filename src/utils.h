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
}
