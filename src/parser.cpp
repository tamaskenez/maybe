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

        U(TokenWord) {}
        U(TokenWspace) {}
        U(TokenNumber) {}
        U(TokenStringLiteral) {}
#undef U
#undef V
    };
    for (; !exit_loop;) {
        auto& token = tokenizer.get_next_token();
        visit(Visitor{this}, token);
    }
    return error_accu;
}
}
