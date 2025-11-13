#include <napi.h>
extern "C" {
  typedef struct {
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

  typedef struct ParserManager ParserManager;

  ParserManager *pm_get(const char *lang_id);
  int pm_set_lang(ParserManager *pm, const char *lang_id);
  void pm_release(ParserManager *pm);

  TokenStream *get_all_visible_tokens(ParserManager *pm,
                                       const char *source,
                                       int x0, int y0, int x1, int y1);
  void free_token_stream(TokenStream *stream);

  int get_languages(const char ***language_list, int *list_size);
}

Napi::Array TokensToJSArray(Napi::Env env, const TokenStream *stream) {
  Napi::Array arr = Napi::Array::New(env, stream->count);
  for (int i = 0; i < stream->count; ++i) {
    const Token &t = stream->tokens[i];
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("x", Napi::Number::New(env, t.x));
    obj.Set("y", Napi::Number::New(env, t.y));
    obj.Set("content", Napi::String::New(env, t.content ? t.content : ""));
    obj.Set("full_path", Napi::String::New(env, t.full_path ? t.full_path : ""));
    obj.Set("full_path_length", Napi::Number::New(env, t.full_path_length));
    arr.Set(i, obj);
  }
  return arr;
}

Napi::Object GetAllVisibleTokens(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  ParserManager *pm = info[0].As<Napi::External<ParserManager>>().Data();
  std::string source = info[1].As<Napi::String>();
  int x0 = info[2].As<Napi::Number>().Int32Value();
  int y0 = info[3].As<Napi::Number>().Int32Value();
  int x1 = info[4].As<Napi::Number>().Int32Value();
  int y1 = info[5].As<Napi::Number>().Int32Value();

  TokenStream *stream = get_all_visible_tokens(pm, source.c_str(), x0, y0, x1, y1);
  Napi::Array tokensJS = TokensToJSArray(env, stream);
  free_token_stream(stream);
  return tokensJS;
}

Napi::Value GetLanguages(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  const char **list = nullptr;
  int size = 0;
  get_languages(&list, &size);
  Napi::Array arr = Napi::Array::New(env, size);
  for (int i = 0; i < size; i++) {
    arr.Set(i, Napi::String::New(env, list[i]));
  }
  return arr;
}

Napi::Value CreateParserManager(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  std::string lang = info[0].As<Napi::String>();
  ParserManager *pm = pm_get(lang.c_str());
  if (!pm) {
    Napi::Error::New(env, "Failed to get ParserManager").ThrowAsJavaScriptException();
    return env.Null();
  }
  return Napi::External<ParserManager>::New(env, pm);
}

Napi::Value ReleaseParserManager(const Napi::CallbackInfo &info) {
  ParserManager *pm = info[0].As<Napi::External<ParserManager>>().Data();
  pm_release(pm);
  return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("createParserManager", Napi::Function::New(env, CreateParserManager));
  exports.Set("releaseParserManager", Napi::Function::New(env, ReleaseParserManager));
  exports.Set("getAllVisibleTokens", Napi::Function::New(env, GetAllVisibleTokens));
  exports.Set("getLanguages", Napi::Function::New(env, GetLanguages));
  return exports;
}

NODE_API_MODULE(tokenizer_addon, Init)
