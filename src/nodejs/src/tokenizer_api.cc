#include <napi.h>
#include <string>

// Try to include parser_manager.h if available
#if __has_include("../../src/parser_manager.h")
#include "../../src/parser_manager.h"
#else
extern "C" {
  typedef struct ParserManager ParserManager;
  typedef struct Token {
    int x;
    int y;
    const char *content;
    const char *full_path;
    int full_path_length;
  } Token;

  typedef struct {
    Token *tokens;
    int count;
  } TokenStream;

  ParserManager *pm_get(const char *lang_id);
  void pm_release(ParserManager *pm);

  TokenStream *get_all_visible_tokens(
      ParserManager *pm,
      const char *source,
      int x0, int y0, int x1, int y1
  );

  void free_token_stream(TokenStream *stream);

  int get_languages(const char ***language_list, int *list_size);
}
#endif

// -----------------------------------------------------------------------------
// JS Class: ParserManager
// -----------------------------------------------------------------------------
class JsParserManager : public Napi::ObjectWrap<JsParserManager> {
public:
  static Napi::FunctionReference constructor;
  ParserManager *pm{nullptr};

  JsParserManager(const Napi::CallbackInfo &info) : Napi::ObjectWrap<JsParserManager>(info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
      Napi::TypeError::New(env, "language id expected").ThrowAsJavaScriptException();
    std::string lang = info[0].As<Napi::String>();
    pm = pm_get(lang.c_str());
    if (!pm)
      Napi::Error::New(env, "Failed to create ParserManager").ThrowAsJavaScriptException();
  }

  ~JsParserManager() override {
    if (pm) {
      pm_release(pm);
      pm = nullptr;
    }
  }

  Napi::Value GetAllVisibleTokens(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (!pm)
      return Napi::Array::New(env, 0);

    std::string source = info[0].As<Napi::String>();
    int x0 = info.Length() > 1 ? info[1].As<Napi::Number>().Int32Value() : 0;
    int y0 = info.Length() > 2 ? info[2].As<Napi::Number>().Int32Value() : 0;
    int x1 = info.Length() > 3 ? info[3].As<Napi::Number>().Int32Value() : 9999;
    int y1 = info.Length() > 4 ? info[4].As<Napi::Number>().Int32Value() : 9999;

    TokenStream *stream = get_all_visible_tokens(pm, source.c_str(), x0, y0, x1, y1);
    Napi::Array arr = Napi::Array::New(env, stream->count);

    for (int i = 0; i < stream->count; ++i) {
      const Token &t = stream->tokens[i];
      Napi::Object o = Napi::Object::New(env);
      o.Set("x", t.x);
      o.Set("y", t.y);
      o.Set("content", t.content ? t.content : "");
      o.Set("full_path", t.full_path ? t.full_path : "");
      o.Set("full_path_length", t.full_path_length);
      arr[i] = o;
    }

    free_token_stream(stream);
    return arr;
  }

  Napi::Value Close(const Napi::CallbackInfo &info) {
    if (pm) {
      pm_release(pm);
      pm = nullptr;
    }
    return info.Env().Undefined();
  }

  static void Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ParserManager", {
      InstanceMethod("getAllVisibleTokens", &JsParserManager::GetAllVisibleTokens),
      InstanceMethod("close", &JsParserManager::Close)
    });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("ParserManager", func);
  }
};

Napi::FunctionReference JsParserManager::constructor;

// -----------------------------------------------------------------------------
// Stand-alone functions
// -----------------------------------------------------------------------------
Napi::Value GetLanguages(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  const char **list = nullptr;
  int count = 0;
  get_languages(&list, &count);
  Napi::Array langs = Napi::Array::New(env, count);
  for (int i = 0; i < count; i++)
    langs[i] = Napi::String::New(env, list[i]);
  return langs;
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  JsParserManager::Init(env, exports);
  exports.Set("getLanguages", Napi::Function::New(env, GetLanguages));
  return exports;
}

NODE_API_MODULE(tokenizer_addon, InitAll)
