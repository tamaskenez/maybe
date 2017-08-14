#include "parser.h"

namespace maybe {

const Token& TokenizerAccess::read_next()
{
    if (UL_LIKELY(!tokenizer->fifo.empty()))
        tokenizer->fifo.pop_front();
    if (UL_UNLIKELY(tokenizer->fifo.empty()))
        tokenizer->load_at_least(*fr, c_tokenizer_batch_size);

    auto& token = tokenizer->fifo.front();

    if (UL_UNLIKELY(holds_alternative<TokenizerError>(token))) {
        auto& e = get<TokenizerError>(token);
        fmt::print("{}:{}:{}: error: {}\n", filename, e.line_num, e.col, e.msg);
        exit(EXIT_FAILURE);
    }
    if (UL_UNLIKELY(holds_alternative<TokenEof>(token))) {
        if (!fr->is_eof())
            log_fatal("can't read file '{}'", filename);
    }
    return token;
}

void Parser::parse_toplevel_loop(TokenizerAccess& ta)
{
    struct BaseVisitor
    {
        UL_NORETURN void operator()(const TokenizerError&)
        {
            // tokenizer error is handled in TokenizerAccess for now
            CHECK(false, "internal error");
        }
        UL_NORETURN void operator()(const TokenEof&)
        {
            // TokenEof is handle before visitation
            CHECK(false, "internal error");
        }
    };
    struct Visitor : BaseVisitor
    {
        using BaseVisitor::operator();

        Visitor(Parser* that) : that(that) {}

#define V(X) void operator()(const X& t)

        V(TokenEol)
        {
            CHECK(that->line_marker_context == lmc_expect_eol);
            that->line_marker_context = lmc_expect_eol;
        }
        V(TokenChar) {}
        V(TokenWspace) {}
        V(TokenIndent)
        {
            CHECK(that->line_marker_context == lmc_expect_indent);
            // peek next token, if it's eol, just ignore this line

            that->line_marker_context = lmc_expect_eol;

            that->current_indent = t;
        }
        V(TokenIdentifier) {}
        V(TokenUnsigned) {}
        V(TokenDouble) {}
#undef V
    private:
        Parser* that;
    };
    for (;;) {
        auto& token = ta.read_next();
        if (UL_UNLIKELY(holds_alternative<TokenEof>(token)))
            break;
        visit(Visitor{this}, token);
    }
}
}
