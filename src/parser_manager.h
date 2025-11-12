#pragma once
#include <tree_sitter/api.h>

typedef struct {
    TSParser *parser;
    TSTree *tree;
    const TSLanguage *lang;
} ParserManager;

ParserManager *pm_get(const char *lang_id);
void pm_release(ParserManager *pm);
