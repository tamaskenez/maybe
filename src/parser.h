#pragma once

#include "tokenizer.h"
#include "log.h"

namespace maybe {

struct Parser
{
    Parser(Tokenizer& tokenizer) : tokenizer(tokenizer) {}

    ErrorAccu parse_toplevel_loop();

private:
    Tokenizer& tokenizer;

    int current_indent;

    bool exit_loop = false;
    ErrorAccu error_accu;
};
}
