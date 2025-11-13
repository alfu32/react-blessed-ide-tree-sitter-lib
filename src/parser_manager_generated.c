#include <string.h>
#include <tree_sitter/api.h>
#include "parser_manager.h"

// Registry of all compiled-in grammars
extern const TSLanguage *tree_sitter_agda(void);
extern const TSLanguage *tree_sitter_arduino(void);
extern const TSLanguage *tree_sitter_asm(void);
extern const TSLanguage *tree_sitter_bash(void);
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_c_sharp(void);
extern const TSLanguage *tree_sitter_cfengine(void);
extern const TSLanguage *tree_sitter_clojure(void);
extern const TSLanguage *tree_sitter_cmake(void);
extern const TSLanguage *tree_sitter_commonlisp(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_css(void);
extern const TSLanguage *tree_sitter_csv(void);
extern const TSLanguage *tree_sitter_d(void);
extern const TSLanguage *tree_sitter_dart(void);
extern const TSLanguage *tree_sitter_diff(void);
extern const TSLanguage *tree_sitter_dockerfile(void);
extern const TSLanguage *tree_sitter_elisp(void);
extern const TSLanguage *tree_sitter_erlang(void);
extern const TSLanguage *tree_sitter_fsharp(void);
extern const TSLanguage *tree_sitter_glsl(void);
extern const TSLanguage *tree_sitter_go(void);
extern const TSLanguage *tree_sitter_gomod(void);
extern const TSLanguage *tree_sitter_gosum(void);
extern const TSLanguage *tree_sitter_gotmpl(void);
extern const TSLanguage *tree_sitter_groovy(void);
extern const TSLanguage *tree_sitter_haskell(void);
extern const TSLanguage *tree_sitter_haxe(void);
extern const TSLanguage *tree_sitter_html(void);
extern const TSLanguage *tree_sitter_ini(void);
extern const TSLanguage *tree_sitter_java(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_json(void);
extern const TSLanguage *tree_sitter_kotlin(void);
extern const TSLanguage *tree_sitter_llvm(void);
extern const TSLanguage *tree_sitter_lua(void);
extern const TSLanguage *tree_sitter_luap(void);
extern const TSLanguage *tree_sitter_luau(void);
extern const TSLanguage *tree_sitter_make(void);
extern const TSLanguage *tree_sitter_markdown(void);
extern const TSLanguage *tree_sitter_mermaid(void);
extern const TSLanguage *tree_sitter_nim(void);
extern const TSLanguage *tree_sitter_objc(void);
extern const TSLanguage *tree_sitter_ocaml(void);
extern const TSLanguage *tree_sitter_ocamllex(void);
extern const TSLanguage *tree_sitter_odin(void);
extern const TSLanguage *tree_sitter_pascal(void);
extern const TSLanguage *tree_sitter_perl(void);
extern const TSLanguage *tree_sitter_php(void);
extern const TSLanguage *tree_sitter_powershell(void);
extern const TSLanguage *tree_sitter_problog(void);
extern const TSLanguage *tree_sitter_properties(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_r(void);
extern const TSLanguage *tree_sitter_regex(void);
extern const TSLanguage *tree_sitter_rescript(void);
extern const TSLanguage *tree_sitter_robot(void);
extern const TSLanguage *tree_sitter_rst(void);
extern const TSLanguage *tree_sitter_ruby(void);
extern const TSLanguage *tree_sitter_rust(void);
extern const TSLanguage *tree_sitter_scala(void);
extern const TSLanguage *tree_sitter_scheme(void);
extern const TSLanguage *tree_sitter_svelte(void);
extern const TSLanguage *tree_sitter_tcl(void);
extern const TSLanguage *tree_sitter_typescript(void);
extern const TSLanguage *tree_sitter_v(void);
extern const TSLanguage *tree_sitter_xml(void);
extern const TSLanguage *tree_sitter_yaml(void);
extern const TSLanguage *tree_sitter_zig(void);

int pm_set_lang(ParserManager *pm, const char *lang_id) {
    if (!pm || !lang_id) return -1;
    if (strcmp(lang_id, "agda") == 0) pm->lang = tree_sitter_agda();
    else if (strcmp(lang_id, "arduino") == 0) pm->lang = tree_sitter_arduino();
    else if (strcmp(lang_id, "asm") == 0) pm->lang = tree_sitter_asm();
    else if (strcmp(lang_id, "bash") == 0) pm->lang = tree_sitter_bash();
    else if (strcmp(lang_id, "c") == 0) pm->lang = tree_sitter_c();
    else if (strcmp(lang_id, "c_sharp") == 0) pm->lang = tree_sitter_c_sharp();
    else if (strcmp(lang_id, "cfengine") == 0) pm->lang = tree_sitter_cfengine();
    else if (strcmp(lang_id, "clojure") == 0) pm->lang = tree_sitter_clojure();
    else if (strcmp(lang_id, "cmake") == 0) pm->lang = tree_sitter_cmake();
    else if (strcmp(lang_id, "commonlisp") == 0) pm->lang = tree_sitter_commonlisp();
    else if (strcmp(lang_id, "cpp") == 0) pm->lang = tree_sitter_cpp();
    else if (strcmp(lang_id, "css") == 0) pm->lang = tree_sitter_css();
    else if (strcmp(lang_id, "csv") == 0) pm->lang = tree_sitter_csv();
    else if (strcmp(lang_id, "d") == 0) pm->lang = tree_sitter_d();
    else if (strcmp(lang_id, "dart") == 0) pm->lang = tree_sitter_dart();
    else if (strcmp(lang_id, "diff") == 0) pm->lang = tree_sitter_diff();
    else if (strcmp(lang_id, "dockerfile") == 0) pm->lang = tree_sitter_dockerfile();
    else if (strcmp(lang_id, "elisp") == 0) pm->lang = tree_sitter_elisp();
    else if (strcmp(lang_id, "erlang") == 0) pm->lang = tree_sitter_erlang();
    else if (strcmp(lang_id, "fsharp") == 0) pm->lang = tree_sitter_fsharp();
    else if (strcmp(lang_id, "glsl") == 0) pm->lang = tree_sitter_glsl();
    else if (strcmp(lang_id, "go") == 0) pm->lang = tree_sitter_go();
    else if (strcmp(lang_id, "gomod") == 0) pm->lang = tree_sitter_gomod();
    else if (strcmp(lang_id, "gosum") == 0) pm->lang = tree_sitter_gosum();
    else if (strcmp(lang_id, "gotmpl") == 0) pm->lang = tree_sitter_gotmpl();
    else if (strcmp(lang_id, "groovy") == 0) pm->lang = tree_sitter_groovy();
    else if (strcmp(lang_id, "haskell") == 0) pm->lang = tree_sitter_haskell();
    else if (strcmp(lang_id, "haxe") == 0) pm->lang = tree_sitter_haxe();
    else if (strcmp(lang_id, "html") == 0) pm->lang = tree_sitter_html();
    else if (strcmp(lang_id, "ini") == 0) pm->lang = tree_sitter_ini();
    else if (strcmp(lang_id, "java") == 0) pm->lang = tree_sitter_java();
    else if (strcmp(lang_id, "javascript") == 0) pm->lang = tree_sitter_javascript();
    else if (strcmp(lang_id, "json") == 0) pm->lang = tree_sitter_json();
    else if (strcmp(lang_id, "kotlin") == 0) pm->lang = tree_sitter_kotlin();
    else if (strcmp(lang_id, "llvm") == 0) pm->lang = tree_sitter_llvm();
    else if (strcmp(lang_id, "lua") == 0) pm->lang = tree_sitter_lua();
    else if (strcmp(lang_id, "luap") == 0) pm->lang = tree_sitter_luap();
    else if (strcmp(lang_id, "luau") == 0) pm->lang = tree_sitter_luau();
    else if (strcmp(lang_id, "make") == 0) pm->lang = tree_sitter_make();
    else if (strcmp(lang_id, "markdown") == 0) pm->lang = tree_sitter_markdown();
    else if (strcmp(lang_id, "mermaid") == 0) pm->lang = tree_sitter_mermaid();
    else if (strcmp(lang_id, "nim") == 0) pm->lang = tree_sitter_nim();
    else if (strcmp(lang_id, "objc") == 0) pm->lang = tree_sitter_objc();
    else if (strcmp(lang_id, "ocaml") == 0) pm->lang = tree_sitter_ocaml();
    else if (strcmp(lang_id, "ocamllex") == 0) pm->lang = tree_sitter_ocamllex();
    else if (strcmp(lang_id, "odin") == 0) pm->lang = tree_sitter_odin();
    else if (strcmp(lang_id, "pascal") == 0) pm->lang = tree_sitter_pascal();
    else if (strcmp(lang_id, "perl") == 0) pm->lang = tree_sitter_perl();
    else if (strcmp(lang_id, "php") == 0) pm->lang = tree_sitter_php();
    else if (strcmp(lang_id, "powershell") == 0) pm->lang = tree_sitter_powershell();
    else if (strcmp(lang_id, "problog") == 0) pm->lang = tree_sitter_problog();
    else if (strcmp(lang_id, "properties") == 0) pm->lang = tree_sitter_properties();
    else if (strcmp(lang_id, "python") == 0) pm->lang = tree_sitter_python();
    else if (strcmp(lang_id, "r") == 0) pm->lang = tree_sitter_r();
    else if (strcmp(lang_id, "regex") == 0) pm->lang = tree_sitter_regex();
    else if (strcmp(lang_id, "rescript") == 0) pm->lang = tree_sitter_rescript();
    else if (strcmp(lang_id, "robot") == 0) pm->lang = tree_sitter_robot();
    else if (strcmp(lang_id, "rst") == 0) pm->lang = tree_sitter_rst();
    else if (strcmp(lang_id, "ruby") == 0) pm->lang = tree_sitter_ruby();
    else if (strcmp(lang_id, "rust") == 0) pm->lang = tree_sitter_rust();
    else if (strcmp(lang_id, "scala") == 0) pm->lang = tree_sitter_scala();
    else if (strcmp(lang_id, "scheme") == 0) pm->lang = tree_sitter_scheme();
    else if (strcmp(lang_id, "svelte") == 0) pm->lang = tree_sitter_svelte();
    else if (strcmp(lang_id, "tcl") == 0) pm->lang = tree_sitter_tcl();
    else if (strcmp(lang_id, "typescript") == 0) pm->lang = tree_sitter_typescript();
    else if (strcmp(lang_id, "v") == 0) pm->lang = tree_sitter_v();
    else if (strcmp(lang_id, "xml") == 0) pm->lang = tree_sitter_xml();
    else if (strcmp(lang_id, "yaml") == 0) pm->lang = tree_sitter_yaml();
    else if (strcmp(lang_id, "zig") == 0) pm->lang = tree_sitter_zig();
    else return -1;
    return 0;
}

static const char *langs[] = {
    "agda", "arduino", "asm", "bash", "c", "c_sharp", "cfengine", "clojure", "cmake", "commonlisp", "cpp", "css", "csv", "d", "dart", "diff", "dockerfile", "elisp", "erlang", "fsharp", "glsl", "go", "gomod", "gosum", "gotmpl", "groovy", "haskell", "haxe", "html", "ini", "java", "javascript", "json", "kotlin", "llvm", "lua", "luap", "luau", "make", "markdown", "mermaid", "nim", "objc", "ocaml", "ocamllex", "odin", "pascal", "perl", "php", "powershell", "problog", "properties", "python", "r", "regex", "rescript", "robot", "rst", "ruby", "rust", "scala", "scheme", "svelte", "tcl", "typescript", "v", "xml", "yaml", "zig"
};

/**
 * Enumerate supported languages.
 */
int get_languages(char ***language_list, int *list_size) {
    if (!language_list || !list_size) return -1;
    *list_size = sizeof(langs) / sizeof(langs[0]);
    *language_list = (char **)langs;
    return 0;
}
