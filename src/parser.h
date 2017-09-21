#pragma once

#include "tokenizer.h"
#include "log.h"

namespace maybe {

struct Parser
{
    Parser(TokenSource&& token_source, string filename)
        : token_source(move(token_source)), filename(move(filename))
    {
    }

    bool parse_toplevel_loop();

private:
    void skip_until_indentation_less_or_equal(int linenum);

    TokenSource token_source;

    int current_indent;
    int current_line_num;

    bool exit_loop = false;
    ErrorAccu error_accu;
    string filename;

    friend struct BaseVisitor;
};
}
