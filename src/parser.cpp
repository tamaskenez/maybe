#include "parser.h"

namespace maybe {

enum VisitResult
{
    continue_,

};

ErrorAccu Parser::parse_toplevel_loop()
{
    struct BaseVisitor
    {
        BaseVisitor(Parser* that) : that(that) {}

        void operator()(const ErrorInSourceFile&)
        {
            ++that->error_accu.num_errors;
            that->exit_loop = true;
        }
        void operator()(const TokenEof&) { that->exit_loop = true; }

    protected:
        Parser* that;
    };
    struct Visitor : BaseVisitor
    {
        using BaseVisitor::operator();

        Visitor(Parser* that) : BaseVisitor(that) {}

#define U(X) void operator()(const X&)
#define V(X) void operator()(const X& t)

        U(TokenEol)
        {
            CHECK(that->line_marker_context == lmc_expect_eol);
            that->line_marker_context = lmc_expect_indent;
        }
        U(TokenChar) {}
        U(TokenWspace) {}
        V(TokenIndent)
        {
            CHECK(that->line_marker_context == lmc_expect_indent);

            auto indent_token = t;  // copy out

            // check next token, if it's eol, just ignore this line
            auto& next_token = that->tokenizer.get_next_token();

            if (holds_alternative<TokenEol>(next_token)) {
                // this was an empty line
                return;
            }

            that->line_marker_context = lmc_expect_eol;
            that->current_indent = t;
        }
        U(TokenIdentifier) {}
        U(TokenNumber) {}
#undef U
#undef V
    };
    for (; !exit_loop;) {
        auto& token = t.get_next_token();
        visit(Visitor{this}, token);
    }
    return error_accu;
}
}
