#include "tokenizer.h"
#include "consts.h"

#include <cmath>
#include <climits>

namespace maybe {

const Token& Tokenizer::get_next_token()
{
    if (UL_LIKELY(!fifo.empty()))
        fifo.pop_front();
    if (UL_UNLIKELY(fifo.empty()))
        load_at_least(c_tokenizer_batch_size);

    return fifo.front();
}

void Tokenizer::load_at_least(int n)
{
    while (UL_LIKELY(fifo.size() < n && state != State::eof)) {
        read_next();
    }
}

void Tokenizer::read_next()
{
    switch (state) {
        case State::waiting_for_indent:
            read_indent();
            break;
        case State::within_line:
            read_within_line();
            break;
        case State::eof:
            break;
        default:
            CHECK(false, "internal");
    }
}

void Tokenizer::read_indent()
{
    current_line_start_pos = fr.chars_read();
    ++line_num;
    TokenIndent::IndentChar ic = TokenIndent::IndentChar::space;
    int level = 0;
    for (;;) {
        auto maybe_c = fr.peek_next_char();
        if (UL_UNLIKELY(!maybe_c))
            break;
        char c = *maybe_c;
        if (c == ' ') {
            if (level == 0 || ic == TokenIndent::IndentChar::space) {
                ++level;
                fr.advance();
            } else {
                fifo.emplace_back<ErrorInSourceFile>(
                    filename, "TAB after SPACE in the indentation", line_num,
                    level + 1, 1);
                state = State::eof;
                return;
            }
        } else if (c == '\t') {
            if (level == 0) {
                level = 1;
                ic = TokenIndent::IndentChar::tab;
                fr.advance();
            } else if (ic == TokenIndent::IndentChar::tab) {
                ++level;
                fr.advance();
            } else {
                fifo.emplace_back<ErrorInSourceFile>(
                    filename, "SPACE after TAB in the indentation", line_num,
                    level + 1, 1);
                state = State::eof;
                return;
            }
        } else {
            // by not calling fr.advance() next char stays in filereader
            break;
        }
    }
    // first char after indent
    state = State::within_line;
    fifo.emplace_back<TokenIndent>(line_num, ic, level);
    return;
}

inline bool iswspace_but_not_newline(char c)
{
    return iswspace(c) && !(c == c_ascii_CR || c == c_ascii_LF);
}

void Tokenizer::read_token_identifier(int startcol, string collector)
{
    // [alpha][alnum]* sequence
    // go until not alnum
    for (;;) {
        auto maybe_c = fr.peek_next_char();
        if (maybe_c && isalnum(*maybe_c)) {
            collector += *maybe_c;
            fr.advance();
        } else
            break;
    }
    fifo.emplace_back<TokenIdentifier>(startcol, move(collector));
}

// Call this after "0x" has been read
// Reads and adds a TokenUnsigned, or, if there are no hex digits right after
// "0x"
// adds a TokenUnsigned with zero value and reads a suffix (TokenIdentifier)
// interpreting
// the x_char as the first character of the suffix.
void Tokenizer::read_hex_literal(int startcol, char x_char)
{
    // read until hex digits
    uint64_t hexnumber = 0;
    bool too_long = false;
    for (;;) {
        int digit;
        auto maybe_c = fr.peek_next_char();
        if (UL_LIKELY(maybe_c)) {
            char c = *maybe_c;
            if ('0' <= c && c <= 9) {
                digit = c - '0';
            } else if ('a' <= c && c <= 'f') {
                digit = c - 'a';
            } else if ('A' <= c && c <= 'F') {
                digit = c - 'A';
            } else
                break;
            fr.advance();
            if (UL_UNLIKELY((hexnumber & 0xf000000000000000) != 0)) {
                too_long = true;
                break;
            }
            hexnumber = (hexnumber << 4) | (uint64_t)digit;
        } else
            break;
    }
    // Read either zero, some or too much digits
    int length = fr.chars_read() - startcol;
    if (UL_UNLIKELY(length <= 2)) {
        CHECK(length == 2);  // "0x"
        // add '0' TokenUnsigned and start new token with *maybe_x
        fifo.emplace_back<TokenNumber>(startcol, 1, uint64_t{0});
        read_token_identifier(startcol + 1, string{1, x_char});
        return;
    }
    if (UL_UNLIKELY(too_long)) {
        fifo.emplace_back<ErrorInSourceFile>(filename,
                                             "hex literal exceeds 8 bytes",
                                             line_num, startcol, length);
    }

    fifo.emplace_back<TokenNumber>(startcol, length, uint64_t{0});
}

// the input nneg_literal is the part of number already read,
// e.g. the very first digit of the number
Nonnegative read_number(FileReader& fr, Nonnegative nneg_literal)
{
    for (;;) {
        auto maybe_c = fr.peek_next_char();
        if (!maybe_c || !isdigit(*maybe_c))
            break;
        fr.advance();
        uint64_t digit = *maybe_c - '0';
        if (UL_LIKELY(holds_alternative<uint64_t>(nneg_literal))) {
            auto& x = get<uint64_t>(nneg_literal);
            if (UL_LIKELY(x <= (UINT64_MAX - digit) / 10)) {
                x = 10 * x + digit;
            } else {
                nneg_literal = (long double)10 * x + digit;
            }
        } else {
            auto& x = get<long double>(nneg_literal);
            x = 10 * x + digit;
        }
    }
    return nneg_literal;
}

long double Tokenizer::read_fractional()
{
    strtmp.assign(1, '.');
    for (;;) {
        auto maybe_c = fr.peek_next_char();
        if (!maybe_c || !isdigit(*maybe_c))
            break;
        fr.advance();
        strtmp += *maybe_c;
    }
    char* str_end = nullptr;
    long double y = strtold(strtmp.c_str(), &str_end);
    CHECK(str_end != strtmp && y != HUGE_VALL);
    return y;
}

long double to_long_double(Nonnegative x)
{
    if (holds_alternative<uint64_t>(x))
        return (long double)get<uint64_t>(x);
    else
        return get<long double>(x);
}

string to_string(const Nonnegative& x)
{
    if (holds_alternative<uint64_t>(x))
        return fmt::format("{}", get<uint64_t>(x));
    else
        return fmt::format("{}", get<long double>(x));
}

void Tokenizer::read_token_number(int startcol, char first_char_digit)
{
    // sequence of digits
    if (UL_UNLIKELY(first_char_digit == '0')) {
        // can be a hex constant
        auto maybe_x = fr.peek_next_char();
        if (maybe_x && (*maybe_x == 'x' || *maybe_x == 'X')) {
            fr.advance();
            read_hex_literal(startcol, *maybe_x);
            return;
        }
    }
    // not hexdigit, read digit seq
    // we have already read a digit, pass it to nat reader which
    // reads the rest
    auto nneg_literal = Nonnegative{(uint64_t)(first_char_digit - '0')};
    nneg_literal = read_number(fr, nneg_literal);
    auto maybe_c = fr.peek_next_char();
    string suffix;  // collect chars here we ate but found invalid
                    // later
    if (UL_LIKELY(maybe_c)) {
        if (*maybe_c == '.') {
            fr.advance();
            long double fractional = read_fractional();
            if (fractional != 0.0L)
                nneg_literal = to_long_double(nneg_literal) + fractional;
            maybe_c = fr.peek_next_char();
        }
        if (*maybe_c == 'e' || *maybe_c == 'E') {
            // at this point we ate 'e' or 'E'
            fr.advance();
            suffix += *maybe_c;
            maybe_c = fr.peek_next_char();
            if (UL_LIKELY(maybe_c)) {
                char sign_char = 0;
                if (*maybe_c == '+' || *maybe_c == '-') {
                    sign_char = *maybe_c;
                    fr.advance();
                    suffix += sign_char;
                    maybe_c = fr.peek_next_char();
                }
                // at this point we ate 'e|E' and
                // - either ate '+|-' and peeked next
                // - or peeked next which is not '+|-'
                if (maybe_c && isdigit(*maybe_c)) {
                    // this is the only branch where we found a
                    // valid
                    // scientific notation double literal
                    fr.advance();
                    long double mantissa = to_long_double(nneg_literal);
                    auto nl_exponent = read_number(
                        fr, Nonnegative{(uint64_t)(*maybe_c - '0')});
                    if (UL_UNLIKELY(
                            holds_alternative<long double>(nl_exponent))) {
                        fifo.emplace_back<ErrorInSourceFile>(
                            filename, "exponent is too high", line_num,
                            startcol, fr.chars_read() - startcol);
                        return;
                    }
                    uint64_t u_exponent = get<uint64_t>(nl_exponent);
                    if (u_exponent >= INT_MAX) {
                        fifo.emplace_back<ErrorInSourceFile>(
                            filename, "exponent is too high", line_num,
                            startcol, fr.chars_read() - startcol);
                        return;
                    }
                    int exponent = (int)u_exponent;
                    if (sign_char == '-')
                        exponent = -exponent;
                    long double x = mantissa * pow(10.0L, exponent);
                    long double int_x;
                    auto frac_x = std::modf(x, &int_x);
                    if (frac_x == 0.0L && x <= (long double)UINT64_MAX) {
                        nneg_literal = (uint64_t)x;
                    } else {
                        nneg_literal = (long double)x;
                    }
                }
            }
        }
    }

    if (holds_alternative<uint64_t>(nneg_literal)) {
        fifo.emplace_back<TokenNumber>(startcol, fr.chars_read() - startcol,
                                       nneg_literal);
    } else {
        long double x = get<long double>(nneg_literal);
        if (std::isnan(x)) {
            fifo.emplace_back<ErrorInSourceFile>(filename, "invalid number",
                                                 line_num, startcol,
                                                 fr.chars_read() - startcol);
            return;
        } else if (std::isinf(x)) {
            fifo.emplace_back<ErrorInSourceFile>(filename, "number overflow",
                                                 line_num, startcol,
                                                 fr.chars_read() - startcol);
            return;
        } else {
            fifo.emplace_back<TokenNumber>(startcol, fr.chars_read() - startcol,
                                           x);
        }
    }
    if (!suffix.empty())
        read_token_identifier(fr.chars_read() - suffix.size(), move(suffix));
}

void Tokenizer::read_within_line()
{
    auto maybe_c = fr.next_char();
    if (UL_UNLIKELY(!maybe_c)) {
        if (fr.is_eof())
            fifo.emplace_back<TokenEof>();
        else
            fifo.emplace_back<ErrorInSourceFile>(
                filename, "can't read file", line_num,
                fr.chars_read() - current_line_start_pos);
        state = State::eof;
        return;
    }
    char c = *maybe_c;
    int startcol = fr.chars_read();  // - 1 + 1
    switch (c) {
        case c_ascii_CR: {
            // also accepts CR without LF as EOL
            auto maybe_next_c = fr.peek_next_char();
            if (maybe_next_c && *maybe_next_c == c_ascii_LF) {
                fr.advance();
            }
            state = State::waiting_for_indent;
            fifo.emplace_back<TokenEol>();
            return;
        }
        case c_ascii_LF:
            state = State::waiting_for_indent;
            fifo.emplace_back<TokenEol>();
            return;
        default: {
            if (iswspace(c)) {
                // whitespace sequence
                int idx = 1;
                for (;;) {
                    maybe_c = fr.peek_next_char();
                    if (maybe_c && iswspace_but_not_newline(*maybe_c)) {
                        ++idx;
                        fr.advance();
                    } else
                        break;
                }
                fifo.emplace_back<TokenWspace>(startcol, idx);
            } else if (isalpha(c)) {
                // [alpha][alnum]* sequence
                read_token_identifier(startcol, string{1, c});
            } else if (isdigit(c)) {
                read_token_number(startcol, c);
                return;
            } else {
                fifo.emplace_back<TokenChar>(startcol, *maybe_c);
                return;
            }
        }
    }
}
}
