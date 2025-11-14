#include <napi.h>
#include <string>

extern "C" {
  #include <tree_sitter/api.h>      // real Tree-Sitter definitions
  #include "../../../src/parser_manager.h"
}

// -----------------------------------------------------------------------------
// JS Class: source_code_parser_t
// -----------------------------------------------------------------------------
class JsParserManager : public Napi::ObjectWrap<JsParserManager> {
public:
  static Napi::FunctionReference constructor;
  source_code_parser_t *pm{nullptr};

  JsParserManager(const Napi::CallbackInfo &info) : Napi::ObjectWrap<JsParserManager>(info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
      Napi::TypeError::New(env, "language id expected").ThrowAsJavaScriptException();
    std::string lang = info[0].As<Napi::String>();
    std::string filename = info[1].As<Napi::String>();
    pm = source_code_parser__new(lang.c_str(),filename.c_str());
    if (!pm)
      Napi::Error::New(env, "Failed to create source_code_parser_t").ThrowAsJavaScriptException();
  }

  ~JsParserManager() override {
    if (pm) {
      source_code_parser__free(pm);
      pm = nullptr;
    }
  }

  Napi::Value SetSource(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (!pm)
      return Napi::Array::New(env, 0);
    std::string text = info[0].As<Napi::String>();
    source_code_parser__set_source(pm, text.c_str());
    return info.Env().Undefined();
  }

  Napi::Value GetAllVisibleTokens(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    if (!pm)
      return Napi::Array::New(env, 0);

    int x0 = info.Length() > 1 ? info[0].As<Napi::Number>().Int32Value() : 0;
    int y0 = info.Length() > 2 ? info[1].As<Napi::Number>().Int32Value() : 0;
    int x1 = info.Length() > 3 ? info[2].As<Napi::Number>().Int32Value() : 9999;
    int y1 = info.Length() > 4 ? info[3].As<Napi::Number>().Int32Value() : 9999;

    token_stream_t *stream = source_code_parser__get_all_visible_tokens(pm, x0, y0, x1, y1);
    Napi::Array arr = Napi::Array::New(env, stream->count);

    for (int i = 0; i < stream->count; ++i) {
      const token_t &t = stream->tokens[i];

        // Safely handle C strings
        std::string content = (t.content ? t.content : "");
        std::string node_type = (t.node_type ? t.node_type : "");
        std::string full_path = (t.full_path ? t.full_path : "");

        // Create JS object
        Napi::Object o = Napi::Object::New(env);

        o.Set("x", t.x);
        o.Set("y", t.y);
        o.Set("x1", t.x + static_cast<int>(content.length()));
        o.Set("content", Napi::String::New(env, content));
        o.Set("node_type", Napi::String::New(env, node_type));
        o.Set("full_path", Napi::String::New(env, full_path));
        o.Set("full_path_length", t.full_path_length);

        arr[i] = o;
    }

    token_stream__free(stream);
    return arr;
  }

  Napi::Value Close(const Napi::CallbackInfo &info) {
    if (pm) {
      source_code_parser__free(pm);
      pm = nullptr;
    }
    return info.Env().Undefined();
  }

  static void Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "source_code_parser_t", {
      InstanceMethod("setSource", &JsParserManager::SetSource),
      InstanceMethod("getAllVisibleTokens", &JsParserManager::GetAllVisibleTokens),
      InstanceMethod("close", &JsParserManager::Close)
    });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    exports.Set("SourceCodeParser", func);
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
