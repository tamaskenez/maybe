#include "parser.h"

namespace maybe {

#if 0
        report_error(ErrorInSourceFile{that->filename, "Unexpected token.",
                                       that->current_line_num, x.col,
                                       x.length});
#endif

Either<ErrorInSourceFile, AstNode> Parser::parse_expression_starting_with(
    Token& token)
{
    CHECK(false);
}

#define VARIANT_GET_IF_BLOCK(SUBTYPE, VAR) if (auto px = get_if<SUBTYPE>(&VAR))

Either<ErrorInSourceFile, AstNode> Parser::parse_definition_after_plus()
{
    auto& token = token_source();
    VARIANT_GET_IF_BLOCK(TokenWord, token)
    {
        if (px->s == "fn") {
        }
    }
}

bool Parser::parse_toplevel_loop()
{
    int error_count = 0;
    bool exit_loop = false;
    do {
        auto& token = token_source();
        BEGIN_VISIT_VARIANT_WITH(t)
        IF_VISITED_VARIANT_IS(t, TokenWord)
        {
            switch (t.kind) {
                case TokenWord::identifier:
                case TokenWord::separator:
                case TokenWord::other:
                    parse_expression_starting_with(token);
                    break;
                case TokenWord::operator_:
                    if (t.s == "+") {
                        parse_definition_after_plus();
                    } else
                        parse_expression_starting_with(token);
                    break;
                default:
                    CHECK(false);
            }
        }
        else if constexpr(VISITED_VARIANT_IS(t, TokenWspace) ||
                          VISITED_VARIANT_IS(t, TokenImplicit))
        { /* do nothing */
        }
        else if constexpr(VISITED_VARIANT_IS(t, TokenNumber) ||
                          VISITED_VARIANT_IS(t, TokenStringLiteral))
        {
            parse_expression_starting_with(token);
        }
        else IF_VISITED_VARIANT_IS(t, ErrorInSourceFile)
        {
            report_error(t);
            ++error_count;
        }
        else IF_VISITED_VARIANT_IS(t, TokenEof) { exit_loop = true; }
        else ERROR_VARIANT_VISIT_NOT_EXHAUSTIVE(t);
        END_VISIT_VARIANT(token)
    } while (!exit_loop);
    return error_count == 0;
}
}
