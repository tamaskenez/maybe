#include "compiler.h"

#include "log.h"
#include "filereader.h"
#include "tokenizer.h"

namespace maybe {

void compile_file(string_par filename)
{
    // auto filename = ul::make_span(f.c_str());
    auto lr = FileReader::new_(filename.c_str());
    if (is_left(lr)) {
        log_fatal(left(lr), "can't open file '{}'", filename.c_str());
    }
    auto fr = move(right(lr));
    Tokenizer tokenizer;
    for (;;) {
        auto or_eof_flag = tokenizer.extract_next_token(fr);
        if (UL_UNLIKELY(is_left(or_eof_flag))) {
            auto& e = left(or_eof_flag);
            fmt::print("{}:{}:{}: error: {}\n", filename.c_str(), e.line_num,
                       e.col, e.msg);
            exit(EXIT_FAILURE);
        }
        auto eof_flag = right(or_eof_flag);
        if (UL_UNLIKELY(eof_flag == EofFlag::eof)) {
            if (!fr.is_eof())
                log_fatal("can't read file '{}'", filename.c_str());
            break;
        }
    }
    for (auto& t : tokenizer.tokens) {
#define MATCH(T)                   \
    if (holds_alternative<T>(t)) { \
        auto& x = get<T>(t);
#define ELSE_MATCH(T)                 \
    }                                 \
    else if (holds_alternative<T>(t)) \
    {                                 \
        auto& x = get<T>(t);
#define END_MATCH }

        MATCH(TokenIndent)
        printf("<IND-%d/%d>", x.num_tabs, x.num_spaces);
        ELSE_MATCH(TokenChar)
        printf("[%c]", x.c);
        ELSE_MATCH(TokenWspace)
        printf("_");
        (void)x;
        ELSE_MATCH(TokenIdentifier)
        printf("<%s>", x.s.c_str());
        ELSE_MATCH(TokenEol)
        printf("<EOL>\n");
        (void)x;
        ELSE_MATCH(TokenUnsigned)
        fmt::print("#U{}", x.number);
        ELSE_MATCH(TokenDouble)
        fmt::print("#D{}", x.number);
        END_MATCH
    }
    printf("<EOF>\n");
}

void run_compiler(const CommandLine& cl)
{
    for (auto& f : cl.files)
        compile_file(f);
}
}
