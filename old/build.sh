#!/usr/bin/env bash
set -e

if ! command -v vcpkg >/dev/null; then
  echo "vcpkg not found. Please install it first."
  exit 1
fi

vcpkg install tree-sitter

# Download grammars
mkdir -p grammars
cd grammars
for repo in c cpp python; do
  if [ ! -d "tree-sitter-$repo" ]; then
    git clone https://github.com/tree-sitter/tree-sitter-$repo.git
  fi
done
cd ..

zig build -Doptimize=ReleaseSafe
