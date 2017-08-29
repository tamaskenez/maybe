#pragma once

#include "std.h"
#include "utils.h"

#include "filereader.h"

namespace maybe {

struct TokenIndent
{
    enum class IndentChar
    {
        tab,
        space
    };

    int line_num;
    IndentChar indent_char;
    int level;
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

using Nonnegative = variant<uint64_t, long double>;

struct TokenNumber
{
    int col, length;
    Nonnegative value;
};

using Token =
    variant<TokenEof,  // first must be a cheap class
            TokenEol,
            TokenChar,
            TokenWspace,
            TokenIndent,
            TokenIdentifier,
            TokenNumber,
            ErrorInSourceFile  // error is flattened into Token to avoid
                               // diffult-to-handle 2-level variant
            >;

struct TokenFifo
{
    const Token& front() const
    {
        assert(!empty());
        return tokens.front();
    }
    Token& front()
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
    // filename is for error msgs
    Tokenizer(FileReader& fr, string filename)
        : fr(fr), filename(move(filename))
    {
    }

    const Token& get_next_token();
    void load_at_least(int n);
    void read_next();

    TokenFifo fifo;

private:
    enum class State
    {
        waiting_for_indent,
        within_line,
        eof
    };

    void pop_front();
    void f();

    void read_indent();
    void read_within_line();
    void read_token_identifier(int startcol, string collector);
    void read_hex_literal(int startcol, char x_char);
    void read_token_number(int startcol, char first_char_digit);
    long double read_fractional();

    FileReader& fr;
    string filename;

    State state = State::waiting_for_indent;
    int line_num = 0;  // 1-based, first line increases it to 1
    int current_line_start_pos = 0;
    string strtmp;
};
}
