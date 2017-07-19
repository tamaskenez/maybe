#include "compiler.h"

#include "log.h"
#include "filereader.h"

namespace maybe {

struct TokenChar
{
    TokenChar(const char* cs) : cs(cs) {}
    const char* cs;
};

struct TokenWspace
{
    TokenWspace(cspan cs) : cs(cs) {}

    cspan cs;
};

using Token = variant<TokenChar, TokenWspace>;

struct TokenizedLine
{
    void clear() {}

    struct
    {
        int num_tabs, num_spaces;
    } indent;
    vector<Token> tokens;
};

struct CompileLineError
{
    CompileLineError(string msg, int col) : msg(move(msg)), col(col) {}

    string msg;
    int col;  // 1-based
};

struct CompileLine
{
    Either<CompileLineError, const TokenizedLine*> compile_line(cspan line)
    {
        tl.clear();
        // find indentation: <TAB>*<SPACE>*
        int idx = 0;
        int tabs = 0, spaces = 0;
        while (idx < line.size()) {
            char c = line[idx];
            if (c == ' ') {
                ++spaces;
            } else if (c == '\t') {
                if (spaces == 0)
                    ++tabs;
                else
                    return CompileLineError(
                        "TAB character after SPACE in the indentation",
                        idx + 1);
            } else {
                break;
            }
        }
        tl.indent.num_tabs = tabs;
        tl.indent.num_spaces = spaces;

        for (; idx < line.size();) {
            char c = line[idx];
            if (iswspace(c)) {
                int startidx = idx;
                ++idx;
                while (idx < line.size() && iswspace(line[idx]))
                    ++idx;
                tl.tokens.emplace_back(TokenWspace(
                    cspan(line.begin() + startidx, idx - startidx)));
            } else {
                tl.tokens.emplace_back(TokenChar(line.begin() + idx));
                ++idx;
            }
        }
        return &tl;
    }

private:
    TokenizedLine tl;
};

using mpark::holds_alternative;
using mpark::get;

void compile_file(string_par filename)
{
    // auto filename = ul::make_span(f.c_str());
    auto lr = FileReader::new_(filename.c_str());
    if (is_left(lr)) {
        log_fatal(left(lr), "can't open file '{}'", filename.c_str());
    }
    auto fr = move(right(lr));
    CompileLine cl;
    for (;;) {
        auto maybe_line = fr.read_next_line();
        if (maybe_line) {
            auto or_tokenized_line = cl.compile_line(*maybe_line);
            if (is_left(or_tokenized_line)) {
                auto cle = left(or_tokenized_line);
                log_fatal("{}:{}:{}: error: {}", filename.c_str(),
                          fr.line_num(), cle.col, cle.msg);
            } else {
                fmt::print("line#{}: ", fr.line_num());
                auto& tl = *right(or_tokenized_line);
                for (auto& x : tl.tokens) {
                    if (holds_alternative<TokenChar>(x)) {
                        fmt::print("'{}' ", *get<TokenChar>(x).cs);
                    } else if (holds_alternative<TokenWspace>(x)) {
                        fmt::print("WSPC/{} ", get<TokenWspace>(x).cs.size());
                    } else
                        log_fatal("internal");
                }
                printf("\n");
            }
        } else
            break;
    }
}

void run_compiler(const CommandLine& cl)
{
    for (auto& f : cl.files)
        compile_file(f);
}
}
