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

// TODO should be long double, or introduce another long double token
struct TokenDouble
{
    int col, length;
    long double number;
};

using Token = variant<TokenChar,
                      TokenWspace,
                      TokenIndent,
                      TokenEol,
                      TokenIdentifier,
                      TokenUnsigned,
                      TokenDouble>;

enum class EofFlag
{
    not_eof,
    eof
};

struct Tokenizer
{
    Either<TokenizerError, EofFlag> extract_next_token(FileReader& fr);

    vector<Token> tokens;

private:
    enum class State
    {
        waiting_for_indent,
        within_line
    };

    Either<TokenizerError, EofFlag> read_indent(FileReader& fr);
    Either<TokenizerError, EofFlag> read_within_line(FileReader& fr);
    Either<TokenizerError, EofFlag> read_token_identifier(FileReader& fr,
                                                          int startcol,
                                                          string collector);
    Either<TokenizerError, EofFlag> read_hex_literal(FileReader& fr,
                                                     int startcol,
                                                     char x_char);
    Either<TokenizerError, EofFlag> read_token_number(FileReader& fr,
                                                      int startcol,
                                                      char first_char_digit);
    long double read_fractional(FileReader& fr);

    State state = State::waiting_for_indent;
    int line_num = 0;  // 1-based, first line increases it to 1
    int current_line_start_pos = 0;
    string strtmp;
};
}
