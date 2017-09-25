#include "tokenimplicitinserter.h"

namespace maybe {

Token& TokenImplicitInserter::get_next_token()
{
    switch (fifo.size()) {
        case 1:
            fifo.pop_front();
        // fall-through
        case 0:
            break;
        default:
            fifo.pop_front();
            return fifo.front();
    }

    auto& token = token_source();
    BEGIN_VISIT_VARIANT_WITH(x)
    IF_VISITED_VARIANT_IS(x, TokenWspace)
    {
        if (!x.inline_()) {
            const auto& top = stack.back();
            if (x.indent_level < top.indent_level) {
                // close all pending blocks with indent level bigger than
                // x.indent_level
                do {
                    CHECK(stack.size() >
                          1);  // we expect this because stack[0] has
                               // a fixed zero indent level,
                               // x.indent_level cannot be less so it
                               // never will be removed
                    if (stack.back().region == region_indent_block) {
                        stack.pop_back();
                        fifo.emplace_back<TokenImplicit>(
                            x.col, x.line_num, TokenImplicit::end_block);
                    } else {
                        // this will be an error in the parser: closing a block
                        // without closing the parens/brackets/braces in the
                        // block
                        stack.pop_back();
                    }
                } while (x.indent_level < stack.back().indent_level);
            } else if (x.indent_level > top.indent_level) {
                // open new block
                stack.emplace_back(Item{x.line_num, x.col, x.indent_level,
                                        region_indent_block});
                fifo.emplace_back<TokenImplicit>(x.col, x.line_num,
                                                 TokenImplicit::begin_block);
            } else {
                // sequencing
                fifo.emplace_back<TokenImplicit>(x.col, x.line_num,
                                                 TokenImplicit::sequencing);
            }
        }
    }
    else if constexpr(VISITED_VARIANT_IS(x, TokenWord) ||
                      VISITED_VARIANT_IS(x, TokenNumber) ||
                      VISITED_VARIANT_IS(x, TokenStringLiteral) ||
                      VISITED_VARIANT_IS(x, ErrorInSourceFile))
    {
        // do nothing
    }
    else IF_VISITED_VARIANT_IS(x, TokenImplicit)
    {
        CHECK(false, "TokenImplicit is not expected here.");
    }
    else IF_VISITED_VARIANT_IS(x, TokenEof)
    {
        // close all pending blocks
        while (stack.size() > 1) {
            if (stack.back().region == region_indent_block) {
                stack.pop_back();
                fifo.emplace_back<TokenImplicit>(x.col, x.line_num,
                                                 TokenImplicit::end_block);
            } else {
                // this will be an error in the parser: closing a block
                // without closing the parens/brackets/braces in the
                // block
                stack.pop_back();
            }
        }
        fifo.emplace_back<TokenEof>(x);
    }
    else ERROR_VARIANT_VISIT_NOT_EXHAUSTIVE(x);
    END_VISIT_VARIANT(token)
    return fifo.empty() ? token : fifo.front();
}
}
