{
  "targets": [
    {
      "target_name": "tokenizer_addon",
      "sources": [ "src/tokenizer_api.cc" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../..",
        "../../src",
        "<!@(echo $VCPKG_ROOT)_installed/x64-linux/include"
      ],
      "libraries": [
        "-L../../build",
        "-ltree_sitter_tokenizer"
      ],
      "library_dirs": [
        "../../build"
      ],
      "link_settings": {
        "libraries": [
          "-L../../build",
          "-ltree_sitter_tokenizer"
        ]
      },
      "dependencies": [
        "<!@(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ]
    }
  ]
}