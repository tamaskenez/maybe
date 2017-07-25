#include "tokenizer.h"
#include "consts.h"

#include <cmath>
#include <climits>

namespace maybe {
Either<TokenizerError, EofFlag> Tokenizer::extract_next_token(FileReader& fr)
{
    switch (state) {
        case State::waiting_for_indent:
            return read_indent(fr);
        case State::within_line:
            return read_within_line(fr);
        default:
            CHECK(false, "internal");
    }
    // never here
    return TokenizerError{line_num,
                          fr.chars_read() - current_line_start_pos + 1, 0,
                          "internal tokenizer error"};
}

Either<TokenizerError, EofFlag> Tokenizer::read_indent(FileReader& fr)
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
                return TokenizerError{
                    line_num, spaces + tabs + 1, 1,
                    "TAB character after SPACE in the indentation"};
        } else {
            // by not calling fr.advance() next char stays in filereader
            break;
        }
    }
    // first char after indent
    state = State::within_line;
    tokens.emplace_back(TokenIndent{line_num, tabs, spaces});
    return EofFlag::not_eof;  // if EOF it will be reported by read_within_line
}

inline bool iswspace_but_not_newline(char c)
{
    return iswspace(c) && !(c == c_ascii_CR || c == c_ascii_LF);
}

Either<TokenizerError, EofFlag>
Tokenizer::read_token_identifier(FileReader& fr, int startcol, string collector)
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
    tokens.emplace_back(TokenIdentifier{startcol, move(collector)});
    return EofFlag::not_eof;
}

// Call this after "0x" has been read
// Reads and adds a TokenUnsigned, or, if there are no hex digits right after
// "0x"
// adds a TokenUnsigned with zero value and reads a suffix (TokenIdentifier)
// interpreting
// the x_char as the first character of the suffix.
Either<TokenizerError, EofFlag> Tokenizer::read_hex_literal(FileReader& fr,
                                                            int startcol,
                                                            char x_char)
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
        tokens.emplace_back(TokenUnsigned{startcol, 1, 0});
        return read_token_identifier(fr, startcol + 1, string{1, x_char});
    }
    if (UL_UNLIKELY(too_long)) {
        return TokenizerError{line_num, startcol, length,
                              "hex literal exceeds 8 bytes"};
    }

    tokens.emplace_back(TokenUnsigned{startcol, length, hexnumber});
    return EofFlag::not_eof;
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

Either<TokenizerError, EofFlag> Tokenizer::read_token_number(
    FileReader& fr,
    int startcol,
    char first_char_digit)
{
    // sequence of digits
    if (UL_UNLIKELY(first_char_digit == '0')) {
        // can be a hex constant
        auto maybe_x = fr.peek_next_char();
        if (maybe_x && (*maybe_x == 'x' || *maybe_x == 'X')) {
            fr.advance();
            return read_hex_literal(fr, startcol, *maybe_x);
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
                            holds_alternative<long double>(nl_exponent)))
                        return TokenizerError{line_num, startcol,
                                              fr.chars_read() - startcol,
                                              "exponent is too high"};
                    uint64_t u_exponent = get<uint64_t>(nl_exponent);
                    if (u_exponent >= INT_MAX)
                        return TokenizerError{line_num, startcol,
                                              fr.chars_read() - startcol,
                                              "exponent is too high"};
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

    if (holds_alternative<uint64_t>(nat_literal))
        tokens.emplace_back(TokenUnsigned{startcol, fr.chars_read() - startcol,
                                          get<uint64_t>(nat_literal)});
    else {
        long double x = get<long double>(nat_literal);
        if (std::isnan(x)) {
        } else if (std::isinf(x)) {
        } else {
            tokens.emplace_back(
                TokenDouble{startcol, fr.chars_read() - startcol, x});
        }
    }

    if (suffix.empty())
        return EofFlag::not_eof;
    else
        return read_token_identifier(fr, fr.chars_read() - suffix.size(),
                                     move(suffix));
}

Either<TokenizerError, EofFlag> Tokenizer::read_within_line(FileReader& fr)
{
    auto maybe_c = fr.next_char();
    if (UL_UNLIKELY(!maybe_c))
        return EofFlag::eof;  // means EOF
    char c = *maybe_c;
    int startcol = fr.chars_read();  // - 1 + 1
    switch (c) {
        case c_ascii_CR: {
            // also accepts CR without LF as EOL
            auto maybe_next_c = fr.peek_next_char();
            if (maybe_next_c && *maybe_next_c == c_ascii_LF) {
                fr.advance();
            }
            tokens.emplace_back(TokenEol{});
            state = State::waiting_for_indent;
            return EofFlag::not_eof;
        }
        case c_ascii_LF:
            tokens.emplace_back(TokenEol{});
            state = State::waiting_for_indent;
            return EofFlag::not_eof;
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
                tokens.emplace_back(TokenWspace{startcol, idx});
                return EofFlag::not_eof;
            } else if (isalpha(c)) {
                // [alpha][alnum]* sequence
                return read_token_identifier(fr, startcol, string{1, c});
            } else if (isdigit(c)) {
                return read_token_number(fr, startcol, c);
            } else {
                tokens.emplace_back(TokenChar{startcol, *maybe_c});
                return EofFlag::not_eof;
            }
        }
    }
}
}
