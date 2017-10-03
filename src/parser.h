#pragma once

#include "tokenizer.h"
#include "log.h"

namespace maybe {

struct Parser
{
    static uptr<Parser> new_(TokenSource&& token_source, string filename);

    virtual bool parse_toplevel_loop() = 0;
    virtual ~Parser() {}
};
}
