{
  "targets": [
    {
      "target_name": "tokenizer_addon",
      "sources": [ "src/tokenizer_api.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include_dir\")"
      ],
      "dependencies": [
        "<!@(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "libraries": [
        "/home/devlin/Development/c-tree-sitter/build/libtree_sitter_tokenizer.so"
      ]
    }
  ]
}