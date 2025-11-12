#include "token.h"
#include "parser_manager.h"
#include <tree_sitter/api.h>
#include <stdlib.h>
#include <string.h>

static void collect_tokens(TSNode node, const char *source, TokenStream *stream) {
    if (ts_node_is_null(node)) return;

    if (ts_node_child_count(node) == 0) {
        uint32_t start = ts_node_start_byte(node);
        uint32_t end = ts_node_end_byte(node);
        int len = end - start;
        char *text = strndup(source + start, len);
        stream->tokens = realloc(stream->tokens, sizeof(Token) * (stream->count + 1));
        Token *t = &stream->tokens[stream->count++];
        t->x = ts_node_start_point(node).column;
        t->y = ts_node_start_point(node).row;
        t->content = text;
        t->qualifiers = NULL;
        t->qualifier_count = 0;
    } else {
        uint32_t n = ts_node_child_count(node);
        for (uint32_t i = 0; i < n; i++)
            collect_tokens(ts_node_child(node, i), source, stream);
    }
}

TokenStream get_all_visible_tokens(const char *source, const char *lang_id, int x0, int y0, int x1, int y1) {
    ParserManager *pm = pm_get(lang_id);
    TokenStream stream = {0};
    if (!pm) return stream;

    pm->tree = ts_parser_parse_string(pm->parser, NULL, source, strlen(source));
    TSNode root = ts_tree_root_node(pm->tree);
    collect_tokens(root, source, &stream);

    // TODO: filter tokens by (x0, y0, x1, y1)
    pm_release(pm);
    return stream;
}

TokenStream query_all_visible_tokens(const char *source, const char *lang_id, const char *query_str,
                                     int x0, int y0, int x1, int y1) {
    ParserManager *pm = pm_get(lang_id);
    TokenStream stream = {0};
    if (!pm) return stream;

    pm->tree = ts_parser_parse_string(pm->parser, NULL, source, strlen(source));
    TSNode root = ts_tree_root_node(pm->tree);

    TSQueryError err;
    TSQuery *query = ts_query_new(pm->lang, query_str, strlen(query_str), NULL, &err);
    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, root);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        for (uint32_t i = 0; i < match.capture_count; i++) {
            TSNode node = match.captures[i].node;
            collect_tokens(node, source, &stream);
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    pm_release(pm);
    return stream;
}

int update(const char *source, const char *lang_id, const TokenStream *stream) {
    // placeholder: could diff old tree vs new and refresh tokens
    (void)source; (void)lang_id; (void)stream;
    return 0;
}

int get_languages(char ***language_list, int *list_size) {
    static char *langs[] = {"c", "cpp", "python"};
    *language_list = langs;
    *list_size = 3;
    return 0;
}
