#include "token.h"
#include "parser_manager.h"
#include <tree_sitter/api.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

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
    static char *langs[] = {
        "ada",
        "agda",
        "angular",
        "apex",
        "arduino",
        "asciidoc",
        "asciidoc_inline",
        "asm",
        "astro",
        "authzed",
        "awk",
        "bash",
        "bass",
        "bazelrc",
        "beancount",
        "bibtex",
        "bicep",
        "bitbake",
        "blueprint",
        "bp",
        "brightscript",
        "c",
        "c_sharp",
        "caddy",
        "capnp",
        "cedar",
        "cfengine",
        "circom",
        "clojure",
        "cmake",
        "cobol",
        "comment",
        "commonlisp",
        "context",
        "cooklang",
        "corn",
        "cpon",
        "cpp",
        "cql",
        "crystal",
        "css",
        "csv",
        "cuda",
        "cue",
        "cylc",
        "d",
        "dart",
        "desktop",
        "devicetree",
        "dhall",
        "diff",
        "djot",
        "dockerfile",
        "doxygen",
        "dtd",
        "earthfile",
        "ebnf",
        "editorconfig",
        "eds",
        "elisp",
        "elixir",
        "elm",
        "embedded_template",
        "enforce",
        "erlang",
        "facility",
        "faust",
        "firrtl",
        "fish",
        "fluentbit",
        "foam",
        "fortran",
        "fsh",
        "fsharp",
        "fsharp_signature",
        "func",
        "gap",
        "gaptst",
        "gdscript",
        "git_config",
        "git_rebase",
        "gitattributes",
        "gitcommit",
        "gleam",
        "glsl",
        "gn",
        "gnuplot",
        "go",
        "goctl",
        "godot_resource",
        "gomod",
        "gosum",
        "gotmpl",
        "gpg",
        "gren",
        "groovy",
        "groq",
        "gstlaunch",
        "gularen",
        "hare",
        "haskell",
        "haxe",
        "hcl",
        "heex",
        "helm",
        "hlsl",
        "hlsplaylist",
        "hoon",
        "html",
        "htmldjango",
        "http",
        "hurl",
        "hyprlang",
        "idl",
        "idris",
        "ini",
        "inko",
        "janet_simple",
        "java",
        "javadoc",
        "javascript",
        "jq",
        "jsdoc",
        "json",
        "json5",
        "julia",
        "kcl",
        "kconfig",
        "kdl",
        "koka",
        "kotlin",
        "koto",
        "lalrpop",
        "ledger",
        "leo",
        "linkerscript",
        "liquidsoap",
        "llvm",
        "llvm_mir",
        "lua",
        "luadoc",
        "luap",
        "luau",
        "magik",
        "mail",
        "make",
        "markdown",
        "markdown_inline",
        "mermaid",
        "mlir",
        "modelica",
        "muttrc",
        "nginx",
        "nickel",
        "nim",
        "nois",
        "nqc",
        "objc",
        "ocaml",
        "ocaml_interface",
        "ocaml_type",
        "ocamllex",
        "pascal",
        "pem",
        "perl",
        "pgn",
        "php",
        "php_only",
        "phpdoc",
        "pioasm",
        "pkl",
        "po",
        "poe_filter",
        "pony",
        "powershell",
        "printf",
        "prisma",
        "problog",
        "properties",
        "psv",
        "puppet",
        "purescript",
        "pymanifest",
        "python",
        "ql",
        "qmljs",
        "query",
        "r",
        "racket",
        "ralph",
        "rasi",
        "razor",
        "rbs",
        "re2c",
        "readline",
        "regex",
        "requirements",
        "rescript",
        "robot",
        "robots",
        "roc",
        "ron",
        "rst",
        "ruby",
        "runescript",
        "rust",
        "scala",
        "scheme",
        "sdml",
        "sflog",
        "slang",
        "slim",
        "slint",
        "smithy",
        "sml",
        "snakemake",
        "solidity",
        "soql",
        "sosl",
        "sourcepawn",
        "sql_bigquery",
        "squirrel",
        "ssh_client_config",
        "ssh_config",
        "stan",
        "starlark",
        "supercollider",
        "superhtml",
        "svelte",
        "sway",
        "systemtap",
        "systemverilog",
        "t32",
        "tablegen",
        "tact",
        "tcl",
        "teal",
        "templ",
        "tera",
        "thrift",
        "tlaplus",
        "tmux",
        "toml",
        "tsv",
        "tsx",
        "turtle",
        "typescript",
        "typespec",
        "udev",
        "unison",
        "usd",
        "v",
        "vbnet",
        "vento",
        "verilog",
        "vhdl",
        "vhs",
        "vim",
        "vimdoc",
        "vrl",
        "vue",
        "wgsl_bevy",
        "wing",
        "xcompose",
        "xml",
        "xresources",
        "yaml",
        "yuck",
        "zathurarc",
        "zeek",
        "zig",
        "ziggy",
        "ziggy_schema",
    };
    *language_list = langs;
    *list_size = 355;
    return 0;
}

void free_token_stream(TokenStream *stream) {
    if (!stream || !stream->tokens) return;
    for (int i = 0; i < stream->count; i++) {
        if (stream->tokens[i].content)
            free((void *)stream->tokens[i].content);
        if (stream->tokens[i].qualifiers)
            free(stream->tokens[i].qualifiers);
    }
    free(stream->tokens);
    stream->tokens = NULL;
    stream->count = 0;
}
