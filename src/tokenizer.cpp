#include "tokenizer.h"

#include <cmath>
#include <climits>

#include "consts.h"

#include "fmt/printf.h"

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
    while (UL_LIKELY(fifo.size() < n && !had_eof)) {
        read_next();
    }
}

void Tokenizer::read_next()
{
    if (UL_UNLIKELY(line_num == 0))
        start_reading_line_skip_empty_lines();
    else if (UL_LIKELY(!had_eof))
        continue_reading_line();
}

void Tokenizer::eof_reached(bool aborted_due_to_error)
{
    if (!aborted_due_to_error && !fr.is_eof()) {
        emplace_error("can't read file",
                      fr.chars_read() - current_line_start_pos, 1);
    }
    had_eof = true;
    fifo.emplace_back<TokenEof>(aborted_due_to_error);
}

void Tokenizer::emplace_error(string_par msg, int startcol, int length)
{
    fifo.emplace_back<ErrorInSourceFile>(filename, msg.str(), line_num,
                                         startcol, length);
}

inline bool is_ucnzc(char c)
{
    // TODO use unicode chars and return if not in Z? or C?
    return (0x20 <= c && c <= 0x7f) || (c & 0x80);
}

inline bool is_allowed_char_in_comments(char c)
{
    return is_ucnzc(c) || c == ' '  // TODO: instead of space, test for Zs
           || c == c_ascii_tab;     // TODO: also allow Cf|Cs
}

bool Tokenizer::try_read_from_inline_comment_after_first_char_read(char c)
{
    if (UL_LIKELY(c != c_token_inline_comment[0]))
        return false;
    auto maybe_c = fr.peek_next_char();
    static_assert(c_token_inline_comment.size() == 2, "");
    if (UL_LIKELY(!maybe_c || *maybe_c == c_token_inline_comment[1]))
        return false;

    fr.advance();
    // read until eol
    for (;;) {
        maybe_c = fr.next_char();
        if (UL_UNLIKELY(!maybe_c)) {
            eof_reached(false);
            return true;
        }
        if (UL_UNLIKELY(try_read_eol_after_first_char_read(*maybe_c))) {
            start_reading_line_skip_empty_lines();
            return true;
        }
        if (UL_UNLIKELY(!is_allowed_char_in_comments(*maybe_c))) {
            emplace_error(
                fmt::sprintf("Invalid character in inline comment: 0x%02x",
                             (uint8_t)*maybe_c),
                fr.chars_read(), 1);
            eof_reached(true);
            return true;
        }
    }
    assert(false);  // unreachable
}

void Tokenizer::start_reading_line_skip_empty_lines()
{
    current_line_start_pos = fr.chars_read();
    ++line_num;

    auto maybe_c = fr.peek_next_char();

    // Test if line begins with shell comment token
    if (UL_UNLIKELY(maybe_c && maybe_c == c_token_shell_comment)) {
        fr.advance();
        // read until EOL
        for (;;) {
            maybe_c = fr.next_char();
            if (UL_UNLIKELY(!maybe_c)) {
                eof_reached(false);
                return;
            }
            if (UL_UNLIKELY(try_read_eol_after_first_char_read(*maybe_c))) {
                // tail-recurse
                start_reading_line_skip_empty_lines();
                return;
            }
            if (UL_UNLIKELY(!is_allowed_char_in_comments(*maybe_c))) {
                emplace_error(
                    fmt::sprintf("Invalid character in shell comment: 0x%02x",
                                 (uint8_t)*maybe_c),
                    fr.chars_read(), 1);
                eof_reached(true);
                return;
            }
        }
        assert(false);  // unreachable
    }

    // read an empty or normal line
    int level = 0;
    for (;;) {  // loop for the indentation
        auto maybe_c = fr.peek_next_char();
        if (UL_UNLIKELY(!maybe_c)) {
            eof_reached(
                false);  // no need to push the indentation token, this's
                         // been an empty line
            return;
        }
        char c = *maybe_c;
        if (c == ' ' || c == c_ascii_tab) {
            if (UL_UNLIKELY(!maybe_file_indent_char))
                maybe_file_indent_char = c;
            if (UL_LIKELY(*maybe_file_indent_char == c)) {
                ++level;
                fr.advance();
            } else {
                if (c == ' ')
                    emplace_error("TAB after SPACE used for indentation",
                                  level + 1, 1);
                else
                    emplace_error("SPACE after TAB used for indentation",
                                  level + 1, 1);

                eof_reached(true);
                return;
            }
        } else {
            // by not calling fr.advance() next char stays in filereader
            break;
        }
    }
    // We're after the (possibly none) indentation
    // Continue with newline, inline comment or ucnzc char
    // It can't be eof, we should have detected it above
    maybe_c = fr.next_char();
    assert(maybe_c);

    // test newline
    if (UL_UNLIKELY(try_read_eol_after_first_char_read(*maybe_c))) {
        // blank line, tail-recurse
        start_reading_line_skip_empty_lines();
        return;
    }

    // test inline comment
    if (UL_UNLIKELY(
            try_read_from_inline_comment_after_first_char_read(*maybe_c)))
        return;

    // now it must be an ucnzc char
    if (UL_UNLIKELY(!is_ucnzc(*maybe_c))) {
        emplace_error(
            fmt::sprintf("Invalid character: 0x%02x", (uint8_t)*maybe_c),
            fr.chars_read(), 1);
        eof_reached(true);
        return;
    }

    fifo.emplace_back<TokenWspace>(line_num, level);

    continue_reading_line(*maybe_c);
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
    fifo.emplace_back<TokenWord>(startcol, TokenWord::identifier,
                                 move(collector));
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

void Tokenizer::continue_reading_line()
{
    auto maybe_c = fr.next_char();
    if (UL_UNLIKELY(!maybe_c)) {
        eof_reached(false);
        return;
    }
    continue_reading_line(*maybe_c);
}

bool Tokenizer::try_read_eol_after_first_char_read(char c)
{
    if (UL_UNLIKELY(c == c_ascii_CR)) {
        // also accepts CR without LF
        auto maybe_c = fr.peek_next_char();
        if (!maybe_c)
            return true;  // CR, then EOF
        if (*maybe_c == c_ascii_LF) {
            fr.advance();  // CRLF
        }
        return true;
    } else
        return c == c_ascii_LF;
}

inline bool is_inline_wspace(char c)
{
    return c == c_ascii_tab || c == ' ';
}

inline bool is_operator(char c)
{
    for (const char* p = &c_token_operators[0]; *p; ++p) {
        if (*p == c)
            return true;
    }
    return false;
}
inline bool is_separator(char c)
{
    for (const char* p = &c_token_separators[0]; *p; ++p) {
        if (*p == c)
            return true;
    }
    return false;
}

// call this after reading the backslash
Maybe<char> Tokenizer::maybe_resolve_escape_sequence_in_interpreted_literal()
{
    auto maybe_c = fr.next_char();
    if (!maybe_c) {
        emplace_error("End-of-file in interpreted string literal",
                      fr.chars_read() - current_line_start_pos, 1);
        eof_reached(false);
        return Nothing;
    }
    Maybe<char> result;
    if (*maybe_c != '\'') {
        for (const char* p = &c_tokenizer_escape_sequences[0]; *p; ++p) {
            if (*p == *maybe_c) {
                auto idx = p - &c_tokenizer_escape_sequences[0];
                result = c_tokenizer_resolved_espace_sequences[idx];
                break;
            }
        }
    }
    if (!result) {
        if (isprint(*maybe_c)) {
            emplace_error(
                fmt::sprintf("Invalid escape sequence: \"\\%c\"", *maybe_c),
                fr.chars_read() - current_line_start_pos, 1);
        } else {
            emplace_error(
                fmt::sprintf(
                    "Invalid escape sequence: raw char \\x%02x after backslash",
                    *maybe_c),
                fr.chars_read() - current_line_start_pos, 1);
        }
        eof_reached(false);
    }
    return result;
}

void Tokenizer::continue_reading_line(char c)
{
    if (try_read_eol_after_first_char_read(c)) {
        // tail-recurse
        start_reading_line_skip_empty_lines();
        return;
    }

    int startcol = fr.chars_read();  // - 1 + 1

    // Following can be: <inline-wspace>+, inline comment or ucnzc

    if (is_inline_wspace(c)) {
        // whitespace sequence
        Maybe<char> maybe_c;
        for (;;) {
            maybe_c = fr.next_char();
            if (!maybe_c || !is_inline_wspace(*maybe_c))
                break;
        }
        if (!maybe_c) {
            eof_reached(false);
            return;
        }
        // at this point we're after a number of whitespaces
        // *maybe_c can be ucznc, '//' or EOL or error
        if (try_read_eol_after_first_char_read(c)) {
            // no need to emplace end-of-line whitespace
            start_reading_line_skip_empty_lines();
            return;
        }
        if (try_read_from_inline_comment_after_first_char_read(c))
            return;
        fifo.emplace_back<TokenWspace>(0, 0);
        continue_reading_line(*maybe_c);  // tail-recurse
        return;
    }

    if (UL_UNLIKELY(try_read_from_inline_comment_after_first_char_read(c)))
        return;

    if (UL_UNLIKELY(!is_ucnzc(c))) {
        emplace_error(fmt::sprintf("Invalid character: 0x%02x", (uint8_t)c),
                      fr.chars_read() - startcol, 1);
        eof_reached(true);
        return;
    }

    if (isalpha(c)) {
        // [alpha][alnum]* sequence
        read_token_identifier(startcol, string{1, c});
    } else if (isdigit(c)) {
        read_token_number(startcol, c);
        return;
    } else if (c == '"') {
        // interpreted string literal
        string w;
        for (;;) {
            auto maybe_c = fr.next_char();
            if (!maybe_c) {
                emplace_error("End-of-file in interpreted string literal",
                              fr.chars_read() - current_line_start_pos, 1);
                eof_reached(false);
                return;
            } else if (*maybe_c < 32) {
                emplace_error(
                    fmt::sprintf("Invalid raw character in "
                                 "interpreted string literal: \\x%02x",
                                 *maybe_c),
                    fr.chars_read() - current_line_start_pos, 1);
                eof_reached(false);
                return;
            }
            if (*maybe_c == '"') {
                break;
            } else if (*maybe_c == '\\') {
                auto maybe_escaped_char =
                    maybe_resolve_escape_sequence_in_interpreted_literal();
                if (!maybe_escaped_char) {
                    eof_reached(true);
                    return;
                }
                w += *maybe_escaped_char;
            } else {
                w += *maybe_c;
            }
        }
        fifo.emplace_back<TokenStringLiteral>(startcol, move(w));
    } else if (is_separator(c)) {
        fifo.emplace_back<TokenWord>(startcol, TokenWord::separator,
                                     string{1, c});
    } else if (is_operator(c)) {
        string w{1, c};
        for (;;) {
            auto maybe_c = fr.peek_next_char();
            if (!maybe_c || !is_operator(*maybe_c))
                break;
            w += *maybe_c;
            fr.advance();
        }
        fifo.emplace_back<TokenWord>(startcol, TokenWord::operator_, move(w));
    } else {
        fifo.emplace_back<TokenWord>(startcol, TokenWord::other, string{1, c});
    }
}
}
