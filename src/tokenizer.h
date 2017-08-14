#pragma once

#include "std.h"
#include "utils.h"

#include "filereader.h"

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

struct TokenEof
{
};

struct TokenIdentifier
{
    int col;
    string s;
};

struct TokenUnsigned
{
    int col, length;
    uint64_t number;
};

struct TokenDouble
{
    int col, length;
    long double number;
};

using Token = variant<TokenEof,  // first must be a cheap class
                      TokenEol,
                      TokenChar,
                      TokenWspace,
                      TokenIndent,
                      TokenIdentifier,
                      TokenUnsigned,
                      TokenDouble,
                      TokenizerError  // error is flattened into Token to avoid
                                      // diffult-to-handle 2-level variant
                      >;

struct TokenFifo
{
    const Token& front()
    {
        assert(!empty());
        return tokens.front();
    }
    const Token& at(int ix) const
    {
        assert(0 <= ix && ix < size());
        return tokens[ix];
    }
    int size() const { return tokens.size(); }
    void pop_front()
    {
        assert(!empty());
        tokens.pop_front();
    }
    template <class T, class... Args>
    void emplace_back(Args&&... args)
    {
        tokens.emplace_back(in_place_aggr_type<T>, std::forward<Args>(args)...);
    }
    bool empty() const { return tokens.empty(); }

private:
    // TODO replace with a more cache friendly implementation if bottleneck
    deque<Token> tokens;
};

struct Tokenizer
{
    void load_at_least(FileReader& fr, int n);
    void read_next(FileReader& fr);

    TokenFifo fifo;

private:
    enum class State
    {
        waiting_for_indent,
        within_line
    };

    void pop_front();
    void f();

    void read_indent(FileReader& fr);
    void read_within_line(FileReader& fr);
    void read_token_identifier(FileReader& fr, int startcol, string collector);
    void read_hex_literal(FileReader& fr, int startcol, char x_char);
    void read_token_number(FileReader& fr, int startcol, char first_char_digit);
    long double read_fractional(FileReader& fr);

    State state = State::waiting_for_indent;
    int line_num = 0;  // 1-based, first line increases it to 1
    int current_line_start_pos = 0;
    string strtmp;
};
}
