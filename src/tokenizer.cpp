#include "tokenizer.h"
#include "consts.h"

namespace maybe {
Either<TokenizerError, int> Tokenizer::extract_next_token(cspan buf)
{
    switch (state) {
        case State::waiting_for_indent:
            return read_indent(true, buf);
        case State::within_indent:
            return read_indent(false, buf);
        case State::within_line:
            return read_within_line(buf);
        default:
            CHECK(false, "internal");
    }
    // never here
    return TokenizerError{line_num, col, 0, "internal tokenizer error"};
}

Either<TokenizerError, int> Tokenizer::read_indent(bool waiting_for_indent,
                                                   cspan buf)
{
    ReadIndentState ris;
    int idx = 0;
    if (waiting_for_indent) {
        ++line_num;
        col = 1;
        ris = ReadIndentState{0, 0};
    } else {
        ris = read_indent_state;
    }
    while (idx < buf.size()) {
        char c = buf[idx];
        if (c == ' ') {
            ++ris.spaces;
            ++idx;
        } else if (c == '\t') {
            if (ris.spaces == 0) {
                ++ris.tabs;
                ++idx;
            } else
                return TokenizerError{
                    line_num, col + idx, 1,
                    "TAB character after SPACE in the indentation"};
        } else
            break;
    }

    if (idx == buf.size()) {
        // did not find end of indent in buf
        state = State::within_indent;
        read_indent_state = ris;  // save for next call
    } else {
        // first char after indent
        state = State::within_line;
        tokens.emplace_back(TokenIndent{line_num, ris.tabs, ris.spaces});
    }
    col += idx;
    if (idx == 0)
        return extract_next_token(buf);
    else
        return idx;
}

Either<TokenizerError, int> Tokenizer::read_within_line(cspan buf)
{
    if (buf.empty())
        return 0;
    char c = buf[0];
    switch (c) {
        case c_ascii_CR: {
            // also accepts CR without LF as EOL
            int s = buf.size() > 1 && buf[1] == c_ascii_LF ? 2 : 1;
            tokens.emplace_back(TokenEol{});
            col += s;
            state = State::waiting_for_indent;
            return s;
        }
        case c_ascii_LF:
            tokens.emplace_back(TokenEol{});
            ++col;
            state = State::waiting_for_indent;
            return 1;
        default: {
            if (iswspace(c)) {
                int idx = 1;
                while (idx < buf.size()) {
                    char c = buf[idx];
                    if (c == c_ascii_LF || c == c_ascii_CR || !iswspace(c))
                        break;
                    ++idx;
                }
                if (!tokens.empty() &&
                    holds_alternative<TokenWspace>(tokens.back())) {
                    auto& t = get<TokenWspace>(tokens.back());
                    t.length += idx;
                } else
                    tokens.emplace_back(TokenWspace{col, idx});
                col += idx;
                return idx;
            } else if (isalpha(c)) {
                int idx = 1;
                // go until not alnum
                while (idx < buf.size() && isalnum(buf[idx]))
                    ++idx;
                // error if too long token
                if (idx > c_tokenizer_max_token_length)
                    return TokenizerError{
                        line_num, col, idx,
                        fmt::format("token length exceeds limit ({})",
                                    c_tokenizer_max_token_length)};
                tokens.emplace_back(
                    TokenToken{col, string{buf.begin(), (size_t)idx}});
                col += idx;
                return idx;
            } else {
                tokens.emplace_back(TokenChar{col, buf[0]});
                ++col;
                return 1;
            }
        }
    }
}
}
