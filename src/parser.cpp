#include "parser.h"
#include "std.h"

namespace maybe {

#if 0
        report_error(ErrorInSourceFile{that->filename, "Unexpected token.",
                                       that->current_line_num, x.col,
                                       x.length});
#endif

#define VARIANT_GET_IF_BLOCK(SUBTYPE, VAR) if (auto px = get_if<SUBTYPE>(&VAR))

using AstNodeId = int;

struct AstNode
{
};

struct Ast
{
    deque<AstNode> nodes;
};

struct StructureStackItem
{
};

struct ParseError
{
};

struct Eof
{
};

using OrAstNode = Either<ParseError, AstNode>;

struct ParserImpl : Parser
{
    ParserImpl(TokenSource&& token_source, string filename)
        : token_source(move(token_source)), filename(move(filename))
    {
    }

    void skip_whitespace()
    {
        for (;;) {
            VARIANT_GET_IF_BLOCK(TokenWspace, peek_next_token())
            {
                swallow_pending_token();
                continue;
            }
            else return;
        }
    }

    void read_until_structural_location(int structural_location)
    {
        CHECK(false);
    }

    AstNode parse_expression()
    {
        CHECK(false);
        return AstNode{};
    }

    variant<OrAstNode, Eof> parse_toplevel_expression()
    {
        auto structural_location_before = structure_stack.size();
        skip_whitespace();
        auto& next_token = peek_next_token();
        VARIANT_GET_IF_BLOCK(TokenImplicit, next_token)
        {
            switch (px->kind) {
                case TokenImplicit::sequencing:
                    // empty line, do tail-recursion
                    swallow_pending_token();
                    return parse_toplevel_expression();
                case TokenImplicit::begin_block:
                case TokenImplicit::end_block:
                    // this is invalid here
                    swallow_pending_token();
                    report_error(ErrorInSourceFile::from_flc(
                        "Invalid implicit begin or end block at toplevel.",
                        filename, px->line_num, px->col));
                    read_until_structural_location(structural_location_before);
                    return ParseError{};
                default:
                    CHECK(false);
            }
        }
        VARIANT_GET_IF_BLOCK(TokenWord, next_token)
        {
            switch (px->kind) {
                case TokenWord::identifier:
                case TokenWord::separator:
                case TokenWord::other:
                    return parse_expression();
                case TokenWord::operator_:
                    if (px->s == "+") {
                        swallow_pending_token();
                        return parse_definition_after_plus();
                    } else
                        return parse_expression();
                default:
                    CHECK(false);
            }
        }
        VARIANT_GET_IF_BLOCK(TokenNumber, next_token)
        {
            return parse_expression();
        }
        VARIANT_GET_IF_BLOCK(TokenStringLiteral, next_token)
        {
            return parse_expression();
        }
        VARIANT_GET_IF_BLOCK(TokenEof, next_token) { return Eof{}; }
        VARIANT_GET_IF_BLOCK(ErrorInSourceFile, next_token)
        {
            return ParseError{};
        }
        CHECK(false);
        return AstNode{};
    }

    void handle_toplevel_expr(AstNode node) {}

    virtual bool parse_toplevel_loop() override
    {
        int error_count = 0;
        bool exit_loop = false;
        do {
            auto toplevel_expr = parse_toplevel_expression();
            BEGIN_VISIT_VARIANT_WITH(x)
            IF_VISITED_VARIANT_IS(x, OrAstNode)
            {
                if (is_left(x)) {
                    ++error_count;
                } else {
                    handle_toplevel_expr(right(x));
                }
            }
            else IF_VISITED_VARIANT_IS(x, Eof) { exit_loop = true; }
            else ERROR_VARIANT_VISIT_NOT_EXHAUSTIVE(x);
            END_VISIT_VARIANT(toplevel_expr)
        } while (!exit_loop);
        return error_count == 0;
    }

    OrAstNode parse_expression_starting_with(Token& token) { CHECK(false); }

    void report_error_on_pending(string msg)
    {
        auto& pending_token = peek_next_token();
        report_error(ErrorInSourceFile::from_flcl(
            move(msg), filename, current_or_peeked_line_num, col(pending_token),
            length(pending_token)));
        swallow_pending_token();
    }

    OrAstNode parse_function_argument_in_definition()
    {
        CHECK(false);
        return ParseError{};
    }

    OrAstNode parse_definition_after_plus()
    {
        skip_whitespace();
        auto& next_token = peek_next_token();
        VARIANT_GET_IF_BLOCK(TokenWord, next_token)
        {
            if (px->s == "fn") {
                swallow_pending_token();
                return parse_definition_after_plus_fn();
            }
        }

        report_error_on_pending("Expected: fn, otherwise not implemented.");
        return ParseError{};
    }

    OrAstNode parse_definition_after_plus_fn()
    {
        skip_whitespace();
        string function_name;
        VARIANT_GET_IF_BLOCK(TokenWord, peek_next_token())
        {
            if (px->kind == TokenWord::identifier)
                function_name = move(px->s);
        }

        if (function_name.empty()) {
            report_error_on_pending("Expected: function name.");
            return ParseError{};
        }

        swallow_pending_token();

        bool open_paren_found = false;
        VARIANT_GET_IF_BLOCK(TokenWord, peek_next_token())
        {
            if (px->kind == TokenWord::separator && px->s == "(")
                open_paren_found = true;
        }

        if (!open_paren_found) {
            report_error_on_pending("Expected: '('");
            return ParseError{};
        }

        swallow_pending_token();

        // loop on arguments
        vector<AstNode> fnargs;
        for (;;) {
            skip_whitespace();
            bool comma_found = false;
            VARIANT_GET_IF_BLOCK(TokenWord, peek_next_token())
            {
                if (px->kind == TokenWord::separator) {
                    if (px->s == ")") {
                        swallow_pending_token();
                        break;
                    } else if (px->s == ",") {
                        swallow_pending_token();
                        comma_found = true;
                    }
                }
            }
            if (!fnargs.empty() && !comma_found) {
                report_error_on_pending(
                    "Expected: comma or closing parenthesis.");
                return ParseError{};
            }

            auto or_fnarg = parse_function_argument_in_definition();
            if (is_left(or_fnarg))
                return ParseError{};
            fnargs.emplace_back(move(right(or_fnarg)));
        }
        // Successfully parsed fnargs.
        // Expected: either '= expression' or new block
        CHECK(false);
    }

    Token& next_token()
    {
        if (pending_token) {
            Token* token = pending_token;
            pending_token = nullptr;
            return *token;
        } else {
            Token& result = token_source();
            auto maybe_line_num = maybe::maybe_line_num(result);
            if (maybe_line_num)
                current_or_peeked_line_num = *maybe_line_num;
            return result;
        }
    }
    Token& peek_next_token()
    {
        if (!pending_token) {
            pending_token = &token_source();
            auto maybe_line_num = maybe::maybe_line_num(*pending_token);
            if (maybe_line_num)
                current_or_peeked_line_num = *maybe_line_num;
        }
        return *pending_token;
    }
    void swallow_pending_token()
    {
        CHECK(pending_token);
        pending_token = nullptr;
    }

    TokenSource token_source;

    int current_indent;
    int current_line_num;

    bool exit_loop = false;
    ErrorAccu error_accu;
    string filename;

    vector<StructureStackItem> structure_stack;

    Token* pending_token = nullptr;
    int current_or_peeked_line_num = 0;
};

uptr<Parser> Parser::new_(TokenSource&& token_source, string filename)
{
    return make_unique<ParserImpl>(move(token_source), move(filename));
}
}
