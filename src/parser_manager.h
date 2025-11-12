#pragma once
#include <tree_sitter/api.h>

#ifdef _WIN32
  #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

typedef struct {
    TSParser *parser;
    TSTree *tree;
    const TSLanguage *lang;
} ParserManager;

API_EXPORT ParserManager *pm_get(const char *lang_id);
API_EXPORT void pm_release(ParserManager *pm);
