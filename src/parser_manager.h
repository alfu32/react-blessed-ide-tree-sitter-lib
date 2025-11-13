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
} SymbolAlias;

typedef struct {
    int x;
    int y;
    const char *content;
    const char *node_type;
    const char *full_path;       // "root > node > leaf"
    int full_path_length;
} Token;

typedef struct {
    Token *tokens;
    int count;
} TokenStream;

typedef struct {
    TSParser *parser;
    TSTree *tree;
    const TSLanguage *lang;
    const char* source;
} ParserManager;

API_EXPORT ParserManager *pm_get(const char *lang_id);
API_EXPORT int pm_set_lang(ParserManager * pm,const char *lang_id);
API_EXPORT void pm_release(ParserManager *pm);

API_EXPORT TokenStream *get_all_visible_tokens(
    ParserManager *pm,
    const char *source,
    int x0, int y0, int x1, int y1
);

API_EXPORT int update(
    ParserManager *pm,
    const char *source,
    const TokenStream *stream
);

API_EXPORT int get_languages(char ***language_list, int *list_size);

API_EXPORT void free_token_stream(TokenStream *stream);
