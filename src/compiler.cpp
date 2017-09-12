#include "compiler.h"

#include "log.h"
#include "filereader.h"
#include "tokenizer.h"
#include "parser.h"

namespace maybe {

struct to_string_functor
{
    template <class T>
    string operator()(const T& x) const
    {
        return std::to_string(x);
    }
};

// true on success
ErrorAccu compile_file(string_par filename)
{
    auto lr = FileReader::new_(filename.c_str());
    if (is_left(lr)) {
        report_error(left(lr), "can't open file '{}'", filename.c_str());
        return ErrorAccu{1};
    }
    auto fr = move(right(lr));
    // skip UTF-8 BOM
    {
        static const array<char, 3> c_utf8_bom = {
            {(char)0xef, (char)0xbb, (char)0xbf}};
        fr.read_ahead_at_least(c_utf8_bom.size());
        bool bom_failed = false;
        for (int i = 0; i < c_utf8_bom.size(); ++i) {
            auto c = fr.peek_char_in_read_buf(i);
            if (!c || *c != c_utf8_bom[i]) {
                bom_failed = true;
                break;
            }
        }
        if (!bom_failed) {
            for (int i = 0; i < c_utf8_bom.size(); ++i)
                fr.next_char();
        }
    }
    Tokenizer tokenizer{fr, filename.str()};

#if 1
    Parser parser{tokenizer};
    return parser.parse_toplevel_loop();
#else
    tokenizer.load_at_least(INT_MAX);
    for (int i = 0; i < tokenizer.fifo.size(); ++i) {
        const auto& t = tokenizer.fifo.at(i);
#define MATCH(T)                   \
    if (holds_alternative<T>(t)) { \
        auto& x = get<T>(t);
#define ELSE_MATCH(T)                 \
    }                                 \
    else if (holds_alternative<T>(t)) \
    {                                 \
        auto& x = get<T>(t);
#define END_MATCH }

        MATCH(TokenWspace)
        if (x.inline_()) {
            printf(" ");
        } else {
            string s(x.indent_level, ' ');
            printf("\n%04d%s", x.line_num, s.c_str());
        }
        ELSE_MATCH(TokenWord)
        printf("<%s>", x.s.c_str());
        ELSE_MATCH(TokenNumber)
        printf("#%s", visit(to_string_functor{}, x.value).c_str());
        ELSE_MATCH(TokenStringLiteral)
        printf("\"");
        for (auto c : x.s) {
            if (isprint(c))
                printf("%c", c);
            else
                printf("\\x%02x", c);
        }
        printf("\"");
        ELSE_MATCH(ErrorInSourceFile)
        if (x.has_location()) {
            printf("ERROR in %s: %s:%d:%d:%d\n", x.filename.c_str(),
                   x.msg.c_str(), x.line_num, x.col, x.length);
        } else {
            printf("ERROR in %s: %s\n", x.filename.c_str(), x.msg.c_str());
        }
        END_MATCH
    }
    printf("<EOF>\n");
    return ErrorAccu{};
#endif
}

int run_compiler(const CommandLine& cl)
{
    ErrorAccu ea;
    for (auto& f : cl.files)
        ea += compile_file(f);
    return ea.num_errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
}
