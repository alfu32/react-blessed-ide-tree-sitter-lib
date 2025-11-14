# Tree-Sitter Token Stream Library

## config
edit [dl-libs-csv](dl-libs.csv) to opt in/out of the languages you want to build into the dll/so

then

```bash
# requires python 3.10.x + ( because of type hints. in extremis you can remove those if it doesn't work)
./init.py

```

it will
- ensure and configure vcpkg
- download all the opted-in tree-sitter modules
- generate [parser_manager_generated.c](src/parser_manager_generated.c) based on the options in [dl-libs-csv](dl-libs.csv)


## npm build config

first ensure node-gyp is installed and then install npm deps

```bash
cd src/nodejs
npm install
cd ../../
```

you can test the npm build only after you have already got the [tree_sitter_lib](build/libtree_sitter_tokenizer.so) :


```bash
npm run build
```

## Build

build and test using the script

```bash
./rebuild-and-test.sh
```


## install npm module

## **Option 1 — Publish it as an npm package (recommended)**

This is the standard and maintainable approach for Node 20+.
You’ll distribute your addon as a normal npm package containing:

* the compiled `.node` binary for each platform, or
* the source + `binding.gyp` (so users can build from source).

### **1. Prepare your package**

In your addon folder (e.g. `src/nodejs/`):

```
src/nodejs/
├── binding.gyp
├── package.json
├── index.js
├── src/addon.cc
└── build/Release/tokenizer_addon.node
```

Edit `package.json`:

```json
{
  "name": "tree-sitter-tokenizer",
  "version": "1.0.0",
  "description": "Cross-language Tree-sitter tokenizer addon for Node.js",
  "main": "index.js",
  "gypfile": true,
  "scripts": {
    "install": "node-gyp rebuild"
  },
  "files": [
    "index.js",
    "binding.gyp",
    "src/",
    "build/Release/"
  ],
  "dependencies": {
    "node-addon-api": "^7.0.0"
  },
  "devDependencies": {
    "node-gyp": "^12.1.0"
  },
  "os": [ "linux", "darwin" ],
  "cpu": [ "x64" ],
  "license": "MIT"
}
```

If you want to include prebuilt binaries (so users don’t need a compiler), use **[prebuildify](https://github.com/prebuild/prebuildify)**:

```bash
npm install --save-dev prebuildify
npx prebuildify --napi --strip
```

This produces `prebuilds/<platform>/tokenizer_addon.node`, which npm installs automatically.

---

### **2. Publish**

Log in and publish:

```bash
npm login
npm publish --access public
```

Users can then install and use it anywhere:

```bash
npm install tree-sitter-tokenizer
```

Usage in another project:

```js
const tokenizer = require('tree-sitter-tokenizer');
const pm = tokenizer.createParserManager('c');
const tokens = tokenizer.getAllVisibleTokens(pm, 'int x=1;');
console.log(tokens);
tokenizer.releaseParserManager(pm);
```

✅ Works cross-platform, versioned, and follows the Node packaging ecosystem.

---

## **Option 2 — Bundle manually for local reuse**

If you just want to reuse the addon in a private or monorepo project:

1. Copy the following into your other project:

   ```
   /node_modules/tree-sitter-tokenizer/
       index.js
       build/Release/tokenizer_addon.node
   ```

   or symlink it:

   ```bash
   npm link ../c-tree-sitter/src/nodejs
   ```
2. In the consuming project:

   ```bash
   npm link tree-sitter-tokenizer
   ```

That allows direct reuse without publishing to npm.

---



