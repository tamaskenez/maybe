#pragma once

#include "std.h"
#include "utils.h"

#include "tokenizer.h"

namespace maybe {
class BeginEndTokenInserter
{
public:
    BeginEndTokenInserter(TokenSource&& token_source)
        : token_source(token_source)
    {
        stack.reserve(c_begin_end_token_inserter_initial_stack_capacity);
        stack.push_back(Item{1, 1, 0, region_indent_block});
    }

    Token& get_next_token();

private:
    enum Region
    {
        region_paren,
        region_bracket,
        region_brace_block,
        region_indent_block
    };
    struct Item
    {
        int line_num;
        int col;
        int indent_level;
        Region region;
    };

    TokenSource token_source;
    vector<Item> stack;
    TokenFifo fifo;
};
}
