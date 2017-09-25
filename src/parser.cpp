#include "parser.h"

namespace maybe {

#if 0
        report_error(ErrorInSourceFile{that->filename, "Unexpected token.",
                                       that->current_line_num, x.col,
                                       x.length});
#endif

bool Parser::parse_toplevel_loop()
{
    int error_count = 0;
    bool exit_loop = false;
    do {
        auto& token = token_source();
        BEGIN_VISIT_VARIANT_WITH(t)
        IF_VISITED_VARIANT_IS(t, TokenWord) {}
        else IF_VISITED_VARIANT_IS(t, TokenWspace) {}
        else IF_VISITED_VARIANT_IS(t, TokenNumber) {}
        else IF_VISITED_VARIANT_IS(t, TokenStringLiteral) {}
        else IF_VISITED_VARIANT_IS(t, TokenImplicit) {}
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
