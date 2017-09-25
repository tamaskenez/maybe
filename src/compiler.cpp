#include "compiler.h"

#include "log.h"
#include "filereader.h"
#include "tokenizer.h"
#include "parser.h"
#include "tokenimplicitinserter.h"

namespace maybe {

struct to_string_functor
{
    template <class T>
    string operator()(const T& x) const
    {
        return std::to_string(x);
    }
};

class TokenStreamPrinter
{
public:
    TokenStreamPrinter(TokenSource&& token_source)
        : token_source(move(token_source))
    {
    }
    Token& get_next_token()
    {
        auto& token = token_source();
        BEGIN_VISIT_VARIANT_WITH(x)
        IF_VISITED_VARIANT_IS(x, TokenWspace)
        {
            if (x.inline_()) {
                printf(" ");
            } else {
                string s(x.indent_level, ' ');
                printf("\n%04d%s", x.line_num, s.c_str());
            }
        }
        else IF_VISITED_VARIANT_IS(x, TokenWord)
        {
            printf("<%s>", x.s.c_str());
        }
        else IF_VISITED_VARIANT_IS(x, TokenNumber)
        {
            printf("#%s", visit(to_string_functor{}, x.value).c_str());
        }
        else IF_VISITED_VARIANT_IS(x, TokenStringLiteral)
        {
            printf("\"");
            for (auto c : x.s) {
                if (isprint(c))
                    printf("%c", c);
                else
                    printf("\\x%02x", c);
            }
            printf("\"");
        }
        else IF_VISITED_VARIANT_IS(x, ErrorInSourceFile)
        {
            if (x.has_location()) {
                printf("ERROR in %s: %s:%d:%d:%d\n", x.filename.c_str(),
                       x.msg.c_str(), x.line_num, x.col, x.length);
            } else {
                printf("ERROR in %s: %s\n", x.filename.c_str(), x.msg.c_str());
            }
        }
        else IF_VISITED_VARIANT_IS(x, TokenEof) { printf("<EOF>\n"); }
        else IF_VISITED_VARIANT_IS(x, TokenImplicit)
        {
            switch (x.kind) {
                case TokenImplicit::sequencing:
                    printf("\n$;");
                    break;
                case TokenImplicit::begin_block:
                    printf("\n${");
                    break;
                case TokenImplicit::end_block:
                    printf("\n$}");
                    break;
                default:
                    CHECK(false);
            }
        }
        else ERROR_VARIANT_VISIT_NOT_EXHAUSTIVE(x);
        END_VISIT_VARIANT(token)
        return token;
    }

private:
    TokenSource token_source;
};

// true on success
bool compile_file(string_par filename)
{
    auto lr = FileReader::new_(filename.c_str());
    if (is_left(lr)) {
        report_error(left(lr), "can't open file '{}'", filename.c_str());
        return false;
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

    TokenSource ts1, ts2;
    unique_ptr<TokenStreamPrinter> tsp;
    unique_ptr<Parser> parser;
    TokenImplicitInserter beti(
        [&tokenizer]() -> Token& { return tokenizer.get_next_token(); });
    TokenSource&& tokens_from_tokenizer = [&beti]() -> Token& {
        return beti.get_next_token();
    };
    if (false) {
        parser =
            make_unique<Parser>(move(tokens_from_tokenizer), filename.str());
    } else {
        tsp = make_unique<TokenStreamPrinter>(move(tokens_from_tokenizer));
        parser = make_unique<Parser>(
            [&tsp]() -> Token& { return tsp->get_next_token(); },
            filename.str());
    }
    return parser->parse_toplevel_loop();
}

int run_compiler(const CommandLine& cl)
{
    bool ok = true;
    for (auto& f : cl.files)
        if (!compile_file(f))
            ok = false;
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
}
