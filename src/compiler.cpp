#include "compiler.h"

#include "log.h"
#include "filereader.h"
#include "tokenizer.h"

namespace maybe {

void compile_file(string_par filename)
{
    // auto filename = ul::make_span(f.c_str());
    auto lr = FileReader::new_(filename.c_str());
    if (is_left(lr)) {
        log_fatal(left(lr), "can't open file '{}'", filename.c_str());
    }
    auto fr = move(right(lr));
    cspan readahead_buf;
    bool eof = false;
    Tokenizer tokenizer;
    for (;;) {
        if (!eof && readahead_buf.size() < Tokenizer::c_min_readahead) {
            readahead_buf =
                fr.read(readahead_buf.size(),
                        c_filereader_read_buf_capacity - readahead_buf.size());
            if (readahead_buf.size() < c_filereader_read_buf_capacity) {
                if (fr.is_eof())
                    eof = true;
                else
                    log_fatal("can't read file '{}'", filename.c_str());
            }
        }
        CHECK(!readahead_buf.empty());
        auto or_consumed = tokenizer.extract_next_token(readahead_buf);
        if (is_left(or_consumed)) {
            auto& e = left(or_consumed);
            log_fatal("{} {} {} {}", filename.c_str(), e.msg, e.line_num,
                      e.col);
        }
        auto consumed = right(or_consumed);
        CHECK(0 < consumed && consumed <= readahead_buf.size());
        readahead_buf = cspan{readahead_buf.begin() + consumed,
                              readahead_buf.size() - consumed};
        if (readahead_buf.empty())
            break;
    }
    for (auto& t : tokenizer.ref_tokens()) {
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
        printf("<%c>", x.c);
        ELSE_MATCH(TokenWspace)
        printf("_");
        (void)x;
        ELSE_MATCH(TokenToken)
        printf("<%s>", x.s.c_str());
        ELSE_MATCH(TokenEol)
        printf("<EOL>\n");
        (void)x;
        END_MATCH
    }
}

void run_compiler(const CommandLine& cl)
{
    for (auto& f : cl.files)
        compile_file(f);
}
}
