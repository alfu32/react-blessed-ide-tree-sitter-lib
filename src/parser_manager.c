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

static const SymbolAlias SYMBOL_ALIASES[] = {
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
                                    TokenStream *stream,
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
            free(text);
            return;
        }

        Token *new_array = realloc(stream->tokens, sizeof(Token) * (stream->count + 1));
        if (!new_array) {
            free(text);
            return;
        }
        stream->tokens = new_array;

        Token *t = &stream->tokens[stream->count++];
        memset(t, 0, sizeof(Token));
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

TokenStream *get_all_visible_tokens(
    ParserManager *pm,
    const char *source,
    int x0, int y0, int x1, int y1
) {
    TokenStream *stream = ts_local_alloc(1, sizeof(TokenStream));
    if (!stream || !source )
        return stream;

    pm->tree = ts_parser_parse_string(pm->parser, NULL, source, strlen(source));
    if (!pm->tree) {
        pm_release(pm);
        return stream;
    }

    TSNode root = ts_tree_root_node(pm->tree);
    if (!ts_node_is_null(root))
        collect_tokens_filtered(root, source, stream, x0, y0, x1, y1);
    return stream;
}

int update(ParserManager *pm, const char *source, const TokenStream *stream) {
    if (!pm) return -1;
    pm->tree = ts_parser_parse_string(pm->parser, pm->tree, source, strlen(source));
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
        ts_local_free((void*)t->node_type);
        ts_local_free((void*)t->full_path);
    }
    ts_local_free(stream->tokens);
    ts_local_free(stream);
}