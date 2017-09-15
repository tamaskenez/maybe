#pragma once

#include "tokenizer.h"
#include "log.h"

namespace maybe {

struct Parser
{
    Parser(Tokenizer& tokenizer, string filename)
        : tokenizer(tokenizer), filename(move(filename))
    {
    }

    ErrorAccu parse_toplevel_loop();

private:
    void skip_until_indentation_less_or_equal(int linenum);

    Tokenizer& tokenizer;

    int current_indent;
    int current_line_num;

    bool exit_loop = false;
    ErrorAccu error_accu;
    string filename;

    friend struct BaseVisitor;
};
}
