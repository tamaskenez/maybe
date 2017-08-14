#pragma once

#include "tokenizer.h"
#include "log.h"

namespace maybe {
struct TokenizerAccess
{
    const Token& read_next();

    FileReader* fr;
    Tokenizer* tokenizer;
    const char* filename;
};

struct Parser
{
    void parse_toplevel_loop(TokenizerAccess& ta);

private:
    enum LineMarkerContext
    {
        lmc_expect_indent,
        lmc_expect_eol
    };

    LineMarkerContext line_marker_context = lmc_expect_indent;
    TokenIndent current_indent;
};
}
