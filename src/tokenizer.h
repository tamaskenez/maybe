#pragma once

#include "std.h"
#include "utils.h"

#include "filereader.h"

namespace maybe {

struct TokenWspace
{
    bool inline_() const { return line_num == 0; }
    int col, length;
    int line_num;      // 0 for inline whitespace, > 0 for whitespace between
                       // lines
    int indent_level;  // valid if line_num > 0
};

struct TokenImplicit
{
    enum Kind
    {
        sequencing,
        begin_block,
        end_block,
    };
    int col;
    int line_num;
    Kind kind;
};

struct TokenEof
{
    int col, length;
    int line_num;
    bool aborted_due_to_error;
};

struct TokenWord
{
    enum Kind
    {
        identifier,
        operator_,
        separator,
        other
    };

    int col, length;
    Kind kind;
    string s;
};

struct TokenStringLiteral
{
    int col, length;
    string s;
};

using Nonnegative = variant<uint64_t, long double>;

struct TokenNumber
{
    int col, length;
    Nonnegative value;
};

using Token =
    variant<TokenEof,
            TokenWord,
            TokenWspace,
            TokenNumber,
            TokenStringLiteral,
            TokenImplicit,
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

    Token& get_next_token();
    void load_at_least(int n);
    void read_next();

    TokenFifo fifo;

private:
    void pop_front();

    void read_indent();
    void read_within_line();
    void read_token_identifier(int startcol, string collector);
    void read_hex_literal(int startcol, char x_char);
    void read_token_number(int startcol, char first_char_digit);
    long double read_fractional();
    void eof_reached(bool aborted_due_to_error);
    void start_reading_line_skip_empty_lines();
    void continue_reading_line();
    void continue_reading_line(char c);
    void emplace_error(string_par msg, int startcol, int length);
    bool try_read_eol_after_first_char_read(char c);
    bool try_read_from_inline_comment_after_first_char_read(char c);
    Maybe<char> maybe_resolve_escape_sequence_in_interpreted_literal();
    int cur_col() const { return fr.chars_read() - current_line_start_pos; }

    FileReader& fr;
    string filename;

    bool had_eof = false;
    int line_num = 0;  // 1-based, first line increases it to 1
    int current_line_start_pos = 0;
    string strtmp;
    Maybe<char> maybe_file_indent_char;
};

using TokenSource = std::function<Token&()>;
}
