#include "compiler.h"

#include "log.h"
#include "filereader.h"
#include "tokenizer.h"
#include "parser.h"

namespace maybe {

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

    Parser parser{tokenizer};
    return parser.parse_toplevel_loop();

#if 0
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
