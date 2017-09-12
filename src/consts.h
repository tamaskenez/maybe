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
static const char c_token_separators[] = "`\"'()[]{};:,";
static const char c_token_operators[] = "~!@#$%^&*-=_+\\|./<>?";
static const char c_tokenizer_escape_sequences[] = "abfnrv\\'\"0";
static const char c_tokenizer_resolved_espace_sequences[] = {
    7, 8, 12, 10, 13, 9, 11, 0x5c, 0x27, 0x22, 0};

static_assert(sizeof(c_tokenizer_escape_sequences) ==
                  sizeof(c_tokenizer_resolved_espace_sequences),
              "");

// things not tunable
static const char c_ascii_LF = 0x0a;
static const char c_ascii_CR = 0x0d;
static const char c_ascii_tab = '\t';
}
