#pragma once
#include <stdint.h>
#include "parser_manager.h"
#include <tree_sitter/api.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int x;
    int y;
    const char *content;
    const char **qualifiers;
    int qualifier_count;
} Token;

typedef struct {
    Token *tokens;
    int count;
} TokenStream;

API_EXPORT TokenStream get_all_visible_tokens(
    const char *source,
    const char *lang_id,
    int x0, int y0, int x1, int y1
);

API_EXPORT int update(
    const char *source,
    const char *lang_id,
    const TokenStream *stream
);

API_EXPORT int get_languages(char ***language_list, int *list_size);

API_EXPORT void free_token_stream(TokenStream *stream);
