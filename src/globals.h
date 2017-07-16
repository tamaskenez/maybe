#pragma once

#include "loglevel.h"

namespace maybe {

struct Globals
{
    LogLevel log_level = LogLevel::debug;
};

extern Globals globals;
}
