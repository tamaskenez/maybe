#pragma once

#include "tokenizer.h"
#include "log.h"

namespace maybe {

struct Parser
{
    Parser(Tokenizer& tokenizer) : tokenizer(tokenizer) {}

    ErrorAccu parse_toplevel_loop();

private:
    enum LineMarkerContext
    {
        lmc_expect_indent,
        lmc_expect_eol
    };

    Tokenizer& tokenizer;

    LineMarkerContext line_marker_context = lmc_expect_indent;
    TokenIndent current_indent;

    bool exit_loop = false;
    ErrorAccu error_accu;
};
}
