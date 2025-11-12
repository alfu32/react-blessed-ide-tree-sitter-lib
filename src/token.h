#pragma once
#include <stdint.h>

#ifdef _WIN32
  #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

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

API_EXPORT TokenStream query_all_visible_tokens(
    const char *source,
    const char *lang_id,
    const char *tree_sitter_query,
    int x0, int y0, int x1, int y1
);

API_EXPORT int update(
    const char *source,
    const char *lang_id,
    const TokenStream *stream
);

API_EXPORT int get_languages(char ***language_list, int *list_size);
