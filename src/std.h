#pragma once

#include <vector>
#include <string>
#include <system_error>
#include <utility>
#include <array>
#include <memory>
#include <deque>

#include "nowide/cstdio.hpp"
#include "nowide/cstdlib.hpp"

#include "mpark/variant.hpp"
#include "akrzemi1/optional.hpp"

#include "ul/string_par.h"
#include "ul/span.h"
#include "ul/check.h"
#include "ul/ul.h"

#include "fmt/ostream.h"

template <class T>
struct always_false : std::false_type
{
};

#define BEGIN_VISIT_VARIANT_WITH(ARG) visit([&](auto&&ARG){
#define VISITED_VARIANT_IS(ARG, SUBTYPE) \
    (std::is_same<typename std::decay<decltype(ARG)>::type, SUBTYPE>::value)
#define IF_VISITED_VARIANT_IS(ARG, SUBTYPE) \
    if                                      \
    constexpr(VISITED_VARIANT_IS(ARG, SUBTYPE))
#define END_VISIT_VARIANT(VARIANT) \
    }, VARIANT);
#define ERROR_VARIANT_VISIT_NOT_EXHAUSTIVE(ARG)                        \
    static_assert(                                                     \
        always_false<typename std::decay<decltype(ARG)>::type>::value, \
        "match is not exhaustive");

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
template <class T>
using uptr = std::unique_ptr<T>;
using std::deque;
using std::make_unique;
using mpark::variant;
using mpark::holds_alternative;
using mpark::visit;
using mpark::get;
using mpark::monostate;
using mpark::in_place_type;
using mpark::in_place_aggr_type;
using mpark::get_if;

template <class T>
using Maybe = std::experimental::optional<T>;
constexpr std::experimental::nullopt_t Nothing{
    std::experimental::nullopt_t::init()};

using ul::string_par;
using ul::make_span;
using ul::span;
using ul::cspan;
}
