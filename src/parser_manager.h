#pragma once
#include <tree_sitter/api.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
  #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

// ---------------------------------------------------------------------
// Canonical symbol mapping (LSP + Tree-sitter highlight conventions)
// ---------------------------------------------------------------------

typedef struct {
    const char *symbol;
    const char *canonical;
} symbol_alias_t;

typedef struct {
    int x;
    int y;
    const char *content;
    const char *node_type;
    const char *full_path;       // "root > node > leaf"
    int full_path_length;
} token_t;

typedef struct {
    token_t *tokens;
    int count;
} token_stream_t;

typedef struct {
    TSParser *parser;
    TSTree *tree;
    const TSLanguage *lang;
    const char* source;
} source_code_parser_t;

API_EXPORT source_code_parser_t *source_code_parser__new(const char *lang_id);
API_EXPORT int source_code_parser__set_lang(source_code_parser_t * pm,const char *lang_id);
API_EXPORT void source_code_parser__free(source_code_parser_t *pm);

API_EXPORT token_stream_t *source_code_parser__get_all_visible_tokens(
    source_code_parser_t *pm,
    const char *source,
    int x0, int y0, int x1, int y1
);

API_EXPORT int source_code_parser__update(
    source_code_parser_t *pm,
    const char *source,
    const token_stream_t *stream
);

API_EXPORT int get_languages(char ***language_list, int *list_size);

API_EXPORT void token_stream__free(token_stream_t *stream);
