#pragma once

#include <vector>
#include <string>
#include <system_error>
#include <utility>

#include "nowide/cstdio.hpp"
#include "nowide/cstdlib.hpp"

#include "mpark/variant.hpp"
#include "akrzemi1/optional.hpp"

#include "ul/string_par.h"
#include "ul/span.h"

#include "fmt/ostream.h"

namespace ul {
inline std::ostream& operator<<(std::ostream& os, cspan d)
{
    return os.write(d.begin(), d.size());
}
}

namespace maybe {
using std::vector;
using std::string;
using std::system_error;
using std::system_category;
using std::move;
using mpark::variant;

template <class T>
using Maybe = std::experimental::optional<T>;
constexpr std::experimental::nullopt_t Nothing{
    std::experimental::nullopt_t::init()};

using ul::string_par;
using ul::make_span;
using ul::span;
using ul::cspan;
}
