#include <tree_sitter/api.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser_manager.h"


void *ts_local_alloc(size_t c,size_t n) { return calloc(c,n); }
void ts_local_free(void *p) { free(p); }

source_code_parser_t *source_code_parser__new(const char *lang_id, const char *filename){
    source_code_parser_t *pm = ts_local_alloc(1, sizeof(source_code_parser_t));
    if (!pm)
        return NULL;

    pm->parser = ts_parser_new();
    if (!pm->parser) {
        ts_local_free(pm);
        printf("no parser");
        return NULL;
    }

    int error = source_code_parser__set_lang(pm,lang_id); // or however your language is resolved
    if (error) {
        ts_parser_delete(pm->parser);
        ts_local_free(pm);
        printf("no parser for lang %s",lang_id);
        return NULL;
    }

    ts_parser_set_language(pm->parser, pm->lang);

    // filename handling
    if (filename && *filename) {
        pm->filename = strdup(filename);
    } else {
        pm->filename = strdup("<unnamed>");
    }

    pm->tree = NULL;
    pm->source = NULL;
    pm->source_len = 0;
    return pm;
}

void source_code_parser__set_source(
    source_code_parser_t *pm,
    const char *src
) {
    if (!pm) return;

    ts_local_free((void*)pm->source);
    pm->source_len = src ? strlen(src) : 0;
    pm->source = src ? strdup(src) : NULL;
}

/**
 * Free all dynamic allocations in token_stream_t.
 */
void source_code_parser__free(source_code_parser_t *pm) {
    if (!pm) return;

    if (pm->tree) {
        ts_tree_delete(pm->tree);
        pm->tree = NULL;
    }

    if (pm->parser) {
        ts_parser_delete(pm->parser);
        pm->parser = NULL;
    }

    ts_local_free(pm->source);
    ts_local_free(pm->filename);
    ts_local_free(pm);
}

static const symbol_alias_t SYMBOL_ALIASES[] = {
    {"{", "punctuation.bracket.curly.open"},
    {"}", "punctuation.bracket.curly.close"},
    {"(", "punctuation.bracket.round.open"},
    {")", "punctuation.bracket.round.close"},
    {"[", "punctuation.bracket.square.open"},
    {"]", "punctuation.bracket.square.close"},
    {";", "punctuation.delimiter.semicolon"},
    {",", "punctuation.delimiter.comma"},
    {"\"", "punctuation.delimiter.quot"},
    {"'", "punctuation.delimiter.single_quot"},
    {".", "punctuation.accessor"},
    {"=", "operator.assignment"},
    {"+", "operator.arithmetic.plus"},
    {"-", "operator.arithmetic.minus"},
    {"*", "operator.arithmetic.multiply"},
    {"/", "operator.arithmetic.divide"},
    {"<", "operator.comparison.less"},
    {">", "operator.comparison.greater"},
    {"!", "operator.logical.not"},
    {"&", "operator.logical.and"},
    {"|", "operator.logical.or"},
    {"?", "operator.ternary"},
    {":", "punctuation.delimiter.colon"},
    {"~", "operator.bitwise.not"},
    {"%", "operator.arithmetic.modulo"},
    {NULL, NULL}
};
const char *canonical_symbol_name(const char *sym) {
    for (int i = 0; SYMBOL_ALIASES[i].symbol; i++) {
        if (strcmp(SYMBOL_ALIASES[i].symbol, sym) == 0)
            return SYMBOL_ALIASES[i].canonical;
    }
    return NULL;
}

// ---------------------------------------------------------------------
// Path builder
// ---------------------------------------------------------------------
void append_node_path(char *path, char* last,size_t size, TSNode node) {
    const char *node_type = ts_node_type(node);

    // Determine final name (symbol alias or node type)
    const char *final_name = NULL;
    if (!ts_node_is_named(node)) {
        final_name = canonical_symbol_name(node_type);
        if (!final_name)
            return;  // skip punctuation we don't recognize
    } else {
        final_name = node_type;
    }

    // Sanitize underscores and create CSS-safe form directly into `last`
    size_t i = 0;
    for (; final_name[i] && i < size - 1; ++i)
        last[i] = (final_name[i] == '_') ? '-' : final_name[i];
    last[i] = '\0';

    // Append separator and name into path
    if (strlen(path) > 0)
        strncat(path, " > ", size - strlen(path) - 1);

    strncat(path, last, size - strlen(path) - 1);
}

// ---------------------------------------------------------------------
// Recursive path generator
// ---------------------------------------------------------------------

void build_full_path(char *path, char* last, size_t size, TSNode node) {
    TSNode parent = ts_node_parent(node);
    if (!ts_node_is_null(parent)) {
        build_full_path(path, last, size, parent);
    }
    append_node_path(path, last, size, node);
}

static void collect_tokens_filtered(TSNode node, const char *source,
                                    token_stream_t *stream,
                                    int x0, int y0, int x1, int y1) {
    if (ts_node_is_null(node) || !source || !stream)
        return;

    const char *node_type = ts_node_type(node);
    if (!node_type)
        return;

    TSPoint start = ts_node_start_point(node);
    TSPoint end   = ts_node_end_point(node);

    if ((int)end.row < y0 || (int)start.row > y1) return;
    if ((int)end.column < x0 || (int)start.column > x1) return;

    uint32_t children = ts_node_child_count(node);
    size_t src_len = strlen(source);

    if (children == 0) {
        uint32_t sb = ts_node_start_byte(node);
        uint32_t eb = ts_node_end_byte(node);
        if (sb >= src_len || eb <= sb) return;
        if (eb > src_len) eb = (uint32_t)src_len;

        char *text = strndup(source + sb, eb - sb);
        if (!text) return;

        char full_path[1024] = {0};
        char qualifier[1024] = {0};
        build_full_path(full_path, qualifier, sizeof(full_path), node);
        if (!sizeof(full_path)) {
            ts_local_free(text);
            return;
        }

        token_t *new_array = realloc(stream->tokens, sizeof(token_t) * (stream->count + 1));
        if (!new_array) {
            ts_local_free(text);
            return;
        }
        stream->tokens = new_array;

        token_t *t = &stream->tokens[stream->count++];
        memset(t, 0, sizeof(token_t));
        t->x = start.column;
        t->y = start.row;
        t->content = text;
        t->node_type = strdup(qualifier);
        t->full_path = strdup(full_path);
        t->full_path_length = strlen(full_path);
    } else {
        for (uint32_t i = 0; i < children; i++) {
            TSNode child = ts_node_child(node, i);
            if (!ts_node_is_null(child))
                collect_tokens_filtered(child, source, stream, x0, y0, x1, y1);
        }
    }
}

token_stream_t *source_code_parser__get_all_visible_tokens(
    source_code_parser_t *pm, int x0, int y0, int x1, int y1
) {
    if (!pm || !pm->source)
        return NULL;

    token_stream_t *stream = ts_local_alloc(1, sizeof(token_stream_t));
    if (!stream || !pm->source )
        return stream;
    
    if (pm->tree) {
        ts_tree_delete(pm->tree);
        pm->tree = NULL;
    }

    pm->tree = ts_parser_parse_string(pm->parser, NULL, pm->source, strlen(pm->source));
    if (!pm->tree) {
        source_code_parser__free(pm);
        return stream;
    }

    TSNode root = ts_tree_root_node(pm->tree);
    if (!ts_node_is_null(root))
        collect_tokens_filtered(root, pm->source, stream, x0, y0, x1, y1);
    return stream;
}

int source_code_parser__update(
    source_code_parser_t *pm,
    const token_stream_t *stream
) {
    if (!pm || !pm->source)
        return -1;
    pm->tree = ts_parser_parse_string(pm->parser, pm->tree, pm->source, strlen(pm->source));
    (void)stream;
    return 0;
}

void token_stream__free(token_stream_t *stream){
    if (!stream) return;
    if (stream->tokens) {
        for (int i = 0; i < stream->count; ++i) {
            token_t *t = &stream->tokens[i];
            ts_local_free((void*)t->content);
            ts_local_free((void*)t->node_type);
            ts_local_free((void*)t->full_path);
            t->content = NULL;
            t->node_type = NULL;
            t->full_path = NULL;
            t->full_path_length = 0;
        }
        ts_local_free(stream->tokens);
    }
    stream->tokens = NULL;
    stream->count = 0;
    ts_local_free(stream);
}