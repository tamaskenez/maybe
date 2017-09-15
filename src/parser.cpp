#include "parser.h"

namespace maybe {

enum VisitResult
{
    continue_,

};

struct BaseVisitor
{
    BaseVisitor(Parser* that) : that(that) {}

    template <class T>
    void operator()(const T& x)
    {
        report_error(ErrorInSourceFile{that->filename, "Unexpected token.",
                                       that->current_line_num, x.col,
                                       x.length});
        ++that->error_accu.num_errors;
        that->skip_until_indentation_less_or_equal(that->current_indent);
    }

    void operator()(const ErrorInSourceFile& x)
    {
        report_error(x);
        ++that->error_accu.num_errors;
        that->skip_until_indentation_less_or_equal(that->current_indent);
    }
    void operator()(const TokenEof&) { that->exit_loop = true; }

protected:
    Parser* that;
};

ErrorAccu Parser::parse_toplevel_loop()
{
#define U(X) void operator()(const X&)
#define V(X) void operator()(const X& t)
    struct Visitor : BaseVisitor
    {
        using BaseVisitor::operator();

        Visitor(Parser* that) : BaseVisitor(that) {}

        // U(TokenWord) { printf("here"); }
        V(TokenWspace)
        {
            if (t.inline_()) {
            } else {
                that->current_indent = t.indent_level;
                that->current_line_num = t.line_num;
            }
        }
        // U(TokenNumber) { printf("here"); }
        // U(TokenStringLiteral) { printf("here"); }
    };
    for (; !exit_loop;) {
        auto& token = tokenizer.get_next_token();
        visit(Visitor{this}, token);
    }
    return error_accu;
}
void Parser::skip_until_indentation_less_or_equal(int linenum)
{
    struct Visitor : BaseVisitor
    {
        using BaseVisitor::operator();

        Visitor(Parser* that) : BaseVisitor(that) {}

        // U(TokenWord) { printf("here"); }
        V(TokenWspace)
        {
            if (t.inline_()) {
            } else {
                that->current_indent = t.indent_level;
                that->current_line_num = t.line_num;
            }
        }
        // U(TokenNumber) { printf("here"); }
        // U(TokenStringLiteral) { printf("here"); }
    };
    for (;;) {
        auto& token = tokenizer.get_next_token();
    }
}
#undef U
#undef V
}
