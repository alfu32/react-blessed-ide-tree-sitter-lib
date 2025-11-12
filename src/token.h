#pragma once
#include <stdint.h>

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

TokenStream get_all_visible_tokens(
    const char *source,
    const char *lang_id,
    int x0, int y0, int x1, int y1
);

TokenStream query_all_visible_tokens(
    const char *source,
    const char *lang_id,
    const char *tree_sitter_query,
    int x0, int y0, int x1, int y1
);

int update(
    const char *source,
    const char *lang_id,
    const TokenStream *stream
);

int get_languages(char ***language_list, int *list_size);
