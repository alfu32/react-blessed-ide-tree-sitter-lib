#include <tree_sitter/api.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser_manager.h"

void *ts_local_alloc(size_t c,size_t n) { return calloc(c,n); }
void ts_local_free(void *p) { free(p); }

ParserManager *pm_get(const char *lang_id) {
    ParserManager *pm = ts_local_alloc(1, sizeof(ParserManager));
    pm->parser = ts_parser_new();
    pm_set_lang(pm,lang_id);
    ts_parser_set_language(pm->parser, pm->lang);
    return pm;
}

void pm_release(ParserManager *pm) {
    if (!pm) return;
    if (pm->tree) ts_tree_delete(pm->tree);
    if (pm->parser) ts_parser_delete(pm->parser);
    ts_local_free(pm);
}

static char *build_full_path(TSNode node) {
    const char *names[128];
    int count = 0;

    // walk upward to root safely
    TSNode cur = node;
    while (!ts_node_is_null(cur) && count < 128) {
        const char *type = ts_node_type(cur);
        if (!type) break;
        names[count++] = type;
        cur = ts_node_parent(cur);
    }

    // nothing found
    if (count == 0) {
        char *empty = strdup("unknown");
        return empty;
    }

    // compute total length
    size_t total = 0;
    for (int i = count - 1; i >= 0; i--)
        total += strlen(names[i]) + (i ? 3 : 0); // " > "

    char *path = ts_local_alloc(total + 1,sizeof(char));
    if (!path) return strdup("alloc_error");

    path[0] = '\0';
    for (int i = count - 1; i >= 0; i--) {
        strcat(path, names[i]);
        if (i > 0) strcat(path, " > ");
    }
    return path;
}

static void collect_tokens_filtered(TSNode node, const char *source,
                                    TokenStream *stream,
                                    int x0, int y0, int x1, int y1) {
    if (ts_node_is_null(node)) return;

    const char *type = ts_node_type(node);
    if (!type) return;

    TSPoint start = ts_node_start_point(node);
    TSPoint end   = ts_node_end_point(node);

    if ((int)end.row < y0 || (int)start.row > y1) return;
    if ((int)end.column < x0 || (int)start.column > x1) return;

    uint32_t children = ts_node_child_count(node);
    if (children == 0) {
        uint32_t sb = ts_node_start_byte(node);
        uint32_t eb = ts_node_end_byte(node);
        if (eb <= sb) return;

        // validate byte offsets
        size_t source_len = strlen(source);
        if (sb >= source_len) return;
        if (eb > source_len) eb = (uint32_t)source_len;

        char *text = strndup(source + sb, eb - sb);
        if (!text) return;

        char *path = build_full_path(node);
        if (!path) path = strdup("unknown");

        stream->tokens = realloc(stream->tokens, sizeof(Token) * (stream->count + 1));
        if (!stream->tokens) return;

        Token *t = &stream->tokens[stream->count++];
        memset(t, 0, sizeof(Token));
        t->x = start.column;
        t->y = start.row;
        t->content = text;
        t->full_path = path;
        t->full_path_length = (int)strlen(path);
    } else {
        for (uint32_t i = 0; i < children; i++) {
            TSNode child = ts_node_child(node, i);
            if (!ts_node_is_null(child))
                collect_tokens_filtered(child, source, stream, x0, y0, x1, y1);
        }
    }
}

TokenStream* get_all_visible_tokens(const char *source,
                                   const char *lang_id,
                                   int x0, int y0, int x1, int y1) {
    TokenStream *stream = ts_local_alloc(1, sizeof(TokenStream));
    ParserManager *pm = pm_get(lang_id);
    if (!pm || !source) return stream;

    pm->tree = ts_parser_parse_string(pm->parser, NULL, source, strlen(source));
    TSNode root = ts_tree_root_node(pm->tree);

    collect_tokens_filtered(root, source, stream, x0, y0, x1, y1);
    pm_release(pm);
    return stream;
}

int update(const char *source, const char *lang_id, const TokenStream *stream) {
    ParserManager *pm = pm_get(lang_id);
    if (!pm) return -1;
    pm->tree = ts_parser_parse_string(pm->parser, pm->tree, source, strlen(source));
    pm_release(pm);
    (void)stream;
    return 0;
}

/**
 * Free all dynamic allocations in TokenStream.
 */
void free_token_stream(TokenStream *stream) {
    if (!stream || !stream->tokens) {
        ts_local_free(stream);
        return;
    }
    for (int i = 0; i < stream->count; i++) {
        Token *t = &stream->tokens[i];
        ts_local_free((void*)t->content);
        ts_local_free((void*)t->full_path);
    }
    ts_local_free(stream->tokens);
    ts_local_free(stream);
}