#include "parser_manager.h"
#include <stdlib.h>
#include <string.h>

// registry of languages can be expanded dynamically
extern const TSLanguage *tree_sitter_c();
extern const TSLanguage *tree_sitter_cpp();
extern const TSLanguage *tree_sitter_python();

ParserManager *pm_get(const char *lang_id) {
    ParserManager *pm = calloc(1, sizeof(ParserManager));
    pm->parser = ts_parser_new();
    if (strcmp(lang_id, "c") == 0)
        pm->lang = tree_sitter_c();
    else if (strcmp(lang_id, "cpp") == 0)
        pm->lang = tree_sitter_cpp();
    else if (strcmp(lang_id, "python") == 0)
        pm->lang = tree_sitter_python();
    else
        return NULL;

    ts_parser_set_language(pm->parser, pm->lang);
    return pm;
}

void pm_release(ParserManager *pm) {
    if (!pm) return;
    if (pm->tree) ts_tree_delete(pm->tree);
    if (pm->parser) ts_parser_delete(pm->parser);
    free(pm);
}
