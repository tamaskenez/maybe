#pragma once

namespace maybe {
static const char* const c_program_name = "maybe";
static const int c_filereader_read_buf_capacity = 65000;
static const int c_tokenizer_max_token_length = 1024;
static const int c_tokenizer_min_readahead = c_tokenizer_max_token_length + 1;

static const char c_ascii_LF = 0x0a;
static const char c_ascii_CR = 0x0d;
}
