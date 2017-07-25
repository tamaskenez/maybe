#pragma once

#include <vector>
#include <string>
#include <system_error>
#include <utility>
#include <array>
#include <memory>

#include "nowide/cstdio.hpp"
#include "nowide/cstdlib.hpp"

#include "mpark/variant.hpp"
#include "akrzemi1/optional.hpp"

#include "ul/string_par.h"
#include "ul/span.h"
#include "ul/check.h"

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
using std::array;
using std::unique_ptr;
using mpark::variant;
using mpark::holds_alternative;
using mpark::get;
using mpark::monostate;

template <class T>
using Maybe = std::experimental::optional<T>;
constexpr std::experimental::nullopt_t Nothing{
    std::experimental::nullopt_t::init()};

using ul::string_par;
using ul::make_span;
using ul::span;
using ul::cspan;
}
