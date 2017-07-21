#pragma once

#include "std.h"
#include "utils.h"

namespace maybe {

struct TokenizerError
{
    int line_num;  // 1-based
    int col;       // 1-based
    int length;
    string msg;
};

struct TokenIndent
{
    int line_num;
    int num_tabs;
    int num_spaces;
};

struct TokenChar
{
    int col;
    char c;
};

struct TokenWspace
{
    int col, length;
};

struct TokenEol
{
};

struct TokenToken
{
    int col;
    string s;
};

using Token =
    variant<TokenChar, TokenWspace, TokenIndent, TokenEol, TokenToken>;

struct Tokenizer
{
    // Readahead should be longer than the longest token except for the
    // following tokens
    // the state machine is implemented to handle unlimited lengths
    //- TokenIndent
    //- TokenStringLiteral
    //- TokenWspace
    // also, since comments are whitespace, they don't count
    static const int c_min_readahead = 32;

    Either<TokenizerError, int> extract_next_token(cspan buf);
    vector<Token> ref_tokens() { return tokens; }

private:
    enum class State
    {
        waiting_for_indent,
        within_indent,
        within_line
    };

    struct ReadIndentState
    {
        int tabs;
        int spaces;
    } read_indent_state;

    Either<TokenizerError, int> read_indent(bool waiting_for_indent, cspan buf);
    Either<TokenizerError, int> read_within_line(cspan buf);
// find indentation: <TAB>*<SPACE>*

#if 0
    for (; idx < line.size();) {
        char c = line[idx];
        if (iswspace(c)) {
            int startidx = idx;
            ++idx;
            while (idx < line.size() && iswspace(line[idx]))
                ++idx;
            tl.tokens.emplace_back(
                TokenWspace(cspan(line.begin() + startidx, idx - startidx)));
        } else {
            tl.tokens.emplace_back(TokenChar(line.begin() + idx));
            ++idx;
        }
    }
    return &tl;
}
#endif
    State state = State::waiting_for_indent;
    int line_num = 0;  // 1-based, first line increases it to 1
    int col = INT_MIN;
    vector<Token> tokens;
};
}
