#include "token.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void add_qualifier(Token *t, const char *q) {
    if (!q) return;
    t->qualifiers = realloc(t->qualifiers,
                            sizeof(char*) * (t->qualifier_count + 1));
    t->qualifiers[t->qualifier_count++] = strdup(q);
}

/**
 * Recursive traversal collecting tokens inside visible window.
 */
static void collect_tokens_filtered(TSNode node, const char *source,
                                    TokenStream *stream,
                                    int x0, int y0, int x1, int y1) {
    if (ts_node_is_null(node)) return;

    TSPoint start = ts_node_start_point(node);
    TSPoint end   = ts_node_end_point(node);

    // prune nodes fully outside window
    if ((int)end.row < y0 || (int)start.row > y1) return;
    if ((int)end.column < x0 || (int)start.column > x1) return;

    uint32_t child_count = ts_node_child_count(node);

    if (child_count == 0) {
        uint32_t sb = ts_node_start_byte(node);
        uint32_t eb = ts_node_end_byte(node);
        if (eb <= sb) return;

        char *text = strndup(source + sb, eb - sb);
        stream->tokens = realloc(stream->tokens,
                                 sizeof(Token) * (stream->count + 1));
        Token *t = &stream->tokens[stream->count++];
        memset(t, 0, sizeof(Token));
        t->x = start.column;
        t->y = start.row;
        t->content = text;
        add_qualifier(t, ts_node_type(node));
    } else {
        for (uint32_t i = 0; i < child_count; i++)
            collect_tokens_filtered(ts_node_child(node, i),
                                    source, stream, x0, y0, x1, y1);
    }
}

/**
 * Main API: collect visible tokens by window.
 */
TokenStream get_all_visible_tokens(const char *source,
                                   const char *lang_id,
                                   int x0, int y0, int x1, int y1) {
    TokenStream stream = {0};
    ParserManager *pm = pm_get(lang_id);
    if (!pm || !source) return stream;

    pm->tree = ts_parser_parse_string(pm->parser, NULL, source, strlen(source));
    TSNode root = ts_tree_root_node(pm->tree);

    collect_tokens_filtered(root, source, &stream, x0, y0, x1, y1);

    pm_release(pm);
    return stream;
}

/**
 * Update parser tree state (placeholder for incremental updates).
 */
int update(const char *source, const char *lang_id, const TokenStream *stream) {
    ParserManager *pm = pm_get(lang_id);
    if (!pm) return -1;
    pm->tree = ts_parser_parse_string(pm->parser, pm->tree, source, strlen(source));
    pm_release(pm);
    (void)stream;
    return 0;
}

/**
 * Enumerate supported languages.
 */
int get_languages(char ***language_list, int *list_size) {

    static char *langs[] = {
        "ada","agda","angular","apex","arduino","asciidoc","asciidoc_inline",
        "asm","astro","authzed","awk","bash","bass","bazelrc","beancount","bibtex",
        "bicep","bitbake","blueprint","bp","brightscript","c","c_sharp","caddy","capnp",
        "cedar","cfengine","circom","clojure","cmake","cobol","comment","commonlisp","context","cooklang","corn","cpon","cpp",
        "cql","crystal","css","csv","cuda","cue","cylc","d","dart","desktop",
        "devicetree","dhall","diff","djot","dockerfile","doxygen","dtd","earthfile","ebnf","editorconfig","eds",
        "elisp","elixir","elm","embedded_template","enforce","erlang","facility","faust","firrtl","fish","fluentbit","foam",
        "fortran","fsh","fsharp","fsharp_signature","func","gap","gaptst","gdscript","git_config","git_rebase",
        "gitattributes","gitcommit","gleam","glsl","gn","gnuplot","go","goctl","godot_resource",
        "gomod","gosum","gotmpl","gpg","gren","groovy","groq","gstlaunch",
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
    int count = sizeof(langs)/sizeof(langs[0]);
    *list_size = count;

    char **arr = malloc(sizeof(char*) * count);
    if (!arr) return -1;
    for (int i = 0; i < count; i++)
        arr[i] = strdup(langs[i]);
    *language_list = arr;
    return 0;
}

/**
 * Free all dynamic allocations in TokenStream.
 */
void free_token_stream(TokenStream *stream) {
    if (!stream || !stream->tokens) return;
    for (int i = 0; i < stream->count; i++) {
        Token *t = &stream->tokens[i];
        free((void*)t->content);
        for (int j = 0; j < t->qualifier_count; j++)
            free((void*)t->qualifiers[j]);
        free(t->qualifiers);
    }
    free(stream->tokens);
    stream->tokens = NULL;
    stream->count = 0;
}