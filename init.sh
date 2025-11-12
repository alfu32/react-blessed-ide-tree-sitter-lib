mkdir c-tree-sitter && cd c-tree-sitter
mkdir src && cd src
touch token.h token.c parser_manager.h parser_manager.c api.c
cd ..
mkdir grammars
touch build.zig vcpkg.json build.sh README.md

echo "mkdir c-tree-sitter && cd c-tree-sitter
mkdir src && cd src
touch token.h token.c parser_manager.h parser_manager.c api.c
cd ..
mkdir grammars
touch build.zig vcpkg.json build.sh README.md dl-libs.sh" > init.sh

chmod +x init.sh
chmod +x build.sh
chmod +x dl-libs.sh
git init