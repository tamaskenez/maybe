#pragma once

namespace maybe {

static const char* const c_program_name = "maybe";

// things to tune
static const int c_filereader_read_buf_capacity = 65000;
static const int c_tokenizer_batch_size =
    10;  // number of tokens read in one batch

// tokenizer/parser
const char c_token_shell_comment = '#';
const constexpr array<char, 2> c_token_inline_comment = {{'/', '/'}};
static const char* c_token_separators = "`\"'()[]{};:,";
static const char* c_token_operators = "~!@#$%^&*-=_+\\|./<>?";

// things not tunable
static const char c_ascii_LF = 0x0a;
static const char c_ascii_CR = 0x0d;
static const char c_ascii_tab = '\t';
}
