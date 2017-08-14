#include "tokenizer.h"
#include "consts.h"

#include <cmath>
#include <climits>

namespace maybe {

void Tokenizer::load_at_least(FileReader& fr, int n)
{
    while (fifo.size() < n) {
        read_next(fr);
    }
}

void Tokenizer::read_next(FileReader& fr)
{
    switch (state) {
        case State::waiting_for_indent:
            read_indent(fr);
            break;
        case State::within_line:
            read_within_line(fr);
            break;
        default:
            CHECK(false, "internal");
    }
}

void Tokenizer::read_indent(FileReader& fr)
{
    current_line_start_pos = fr.chars_read();
    ++line_num;
    int tabs = 0;
    int spaces = 0;
    for (;;) {
        auto maybe_c = fr.peek_next_char();
        if (UL_UNLIKELY(!maybe_c))
            break;
        char c = *maybe_c;
        if (c == ' ') {
            ++spaces;
            fr.advance();
        } else if (c == '\t') {
            if (spaces == 0) {
                ++tabs;
                fr.advance();
            } else
                fifo.emplace_back<TokenizerError>(
                    line_num, spaces + tabs + 1, 1,
                    "TAB character after SPACE in the indentation");
            return;
        } else {
            // by not calling fr.advance() next char stays in filereader
            break;
        }
    }
    // first char after indent
    state = State::within_line;
    fifo.emplace_back<TokenIndent>(line_num, tabs, spaces);
    return;
}

inline bool iswspace_but_not_newline(char c)
{
    return iswspace(c) && !(c == c_ascii_CR || c == c_ascii_LF);
}

void Tokenizer::read_token_identifier(FileReader& fr,
                                      int startcol,
                                      string collector)
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
void Tokenizer::read_hex_literal(FileReader& fr, int startcol, char x_char)
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
        fifo.emplace_back<TokenUnsigned>(startcol, 1, 0);
        read_token_identifier(fr, startcol + 1, string{1, x_char});
        return;
    }
    if (UL_UNLIKELY(too_long)) {
        fifo.emplace_back<TokenizerError>(line_num, startcol, length,
                                          "hex literal exceeds 8 bytes");
    }

    fifo.emplace_back<TokenUnsigned>(startcol, length, hexnumber);
}

using NatLiteral = variant<uint64_t, long double>;

// the input nat_literal is the part of number already read,
// e.g. the very first digit of the number
NatLiteral read_number(FileReader& fr, NatLiteral nat_literal)
{
    for (;;) {
        auto maybe_c = fr.peek_next_char();
        if (!maybe_c || !isdigit(*maybe_c))
            break;
        fr.advance();
        uint64_t digit = *maybe_c - '0';
        if (UL_LIKELY(holds_alternative<uint64_t>(nat_literal))) {
            auto& x = get<uint64_t>(nat_literal);
            if (UL_LIKELY(x <= (UINT64_MAX - digit) / 10)) {
                x = 10 * x + digit;
            } else {
                nat_literal = (long double)10 * x + digit;
            }
        } else {
            auto& x = get<long double>(nat_literal);
            x = 10 * x + digit;
        }
    }
    return nat_literal;
}

long double Tokenizer::read_fractional(FileReader& fr)
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

long double to_long_double(NatLiteral x)
{
    if (holds_alternative<uint64_t>(x))
        return (long double)get<uint64_t>(x);
    else
        return get<long double>(x);
}

string to_string(const NatLiteral& x)
{
    if (holds_alternative<uint64_t>(x))
        return fmt::format("{}", get<uint64_t>(x));
    else
        return fmt::format("{}", get<long double>(x));
}

void Tokenizer::read_token_number(FileReader& fr,
                                  int startcol,
                                  char first_char_digit)
{
    // sequence of digits
    if (UL_UNLIKELY(first_char_digit == '0')) {
        // can be a hex constant
        auto maybe_x = fr.peek_next_char();
        if (maybe_x && (*maybe_x == 'x' || *maybe_x == 'X')) {
            fr.advance();
            read_hex_literal(fr, startcol, *maybe_x);
            return;
        }
    }
    // not hexdigit, read digit seq
    // we have already read a digit, pass it to nat reader which
    // reads the rest
    auto nat_literal = NatLiteral{(uint64_t)(first_char_digit - '0')};
    nat_literal = read_number(fr, nat_literal);
    auto maybe_c = fr.peek_next_char();
    string suffix;  // collect chars here we ate but found invalid
                    // later
    if (UL_LIKELY(maybe_c)) {
        if (*maybe_c == '.') {
            fr.advance();
            long double fractional = read_fractional(fr);
            if (fractional != 0.0L)
                nat_literal = to_long_double(nat_literal) + fractional;
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
                    long double mantissa = to_long_double(nat_literal);
                    auto nl_exponent =
                        read_number(fr, NatLiteral{(uint64_t)(*maybe_c - '0')});
                    if (UL_UNLIKELY(
                            holds_alternative<long double>(nl_exponent))) {
                        fifo.emplace_back<TokenizerError>(
                            line_num, startcol, fr.chars_read() - startcol,
                            "exponent is too high");
                        return;
                    }
                    uint64_t u_exponent = get<uint64_t>(nl_exponent);
                    if (u_exponent >= INT_MAX) {
                        fifo.emplace_back<TokenizerError>(
                            line_num, startcol, fr.chars_read() - startcol,
                            "exponent is too high");
                        return;
                    }
                    int exponent = (int)u_exponent;
                    if (sign_char == '-')
                        exponent = -exponent;
                    long double x = mantissa * pow(10.0L, exponent);
                    long double int_x;
                    auto frac_x = std::modf(x, &int_x);
                    if (frac_x == 0.0L && x <= (long double)UINT64_MAX) {
                        nat_literal = (uint64_t)x;
                    } else {
                        nat_literal = (long double)x;
                    }
                }
            }
        }
    }

    if (holds_alternative<uint64_t>(nat_literal)) {
        fifo.emplace_back<TokenUnsigned>(startcol, fr.chars_read() - startcol,
                                         get<uint64_t>(nat_literal));
    } else {
        long double x = get<long double>(nat_literal);
        if (std::isnan(x)) {
            fifo.emplace_back<TokenizerError>(line_num, startcol,
                                              fr.chars_read() - startcol,
                                              "invalid number");
            return;
        } else if (std::isinf(x)) {
            fifo.emplace_back<TokenizerError>(line_num, startcol,
                                              fr.chars_read() - startcol,
                                              "number overflow");
            return;
        } else {
            fifo.emplace_back<TokenDouble>(startcol, fr.chars_read() - startcol,
                                           x);
        }
    }
    if (!suffix.empty())
        read_token_identifier(fr, fr.chars_read() - suffix.size(),
                              move(suffix));
}

void Tokenizer::read_within_line(FileReader& fr)
{
    auto maybe_c = fr.next_char();
    if (UL_UNLIKELY(!maybe_c)) {
        fifo.emplace_back<TokenEof>();
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
                read_token_identifier(fr, startcol, string{1, c});
            } else if (isdigit(c)) {
                read_token_number(fr, startcol, c);
                return;
            } else {
                fifo.emplace_back<TokenChar>(startcol, *maybe_c);
                return;
            }
        }
    }
}
}
