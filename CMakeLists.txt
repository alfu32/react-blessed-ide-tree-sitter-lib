cmake_minimum_required(VERSION 3.22)
project(tree_sitter_tokenizer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- vcpkg integration ---
if(NOT DEFINED ENV{VCPKG_ROOT})
    message(FATAL_ERROR "VCPKG_ROOT not set. Run init.py or export it.")
endif()

set(VCPKG_ROOT "$ENV{VCPKG_ROOT}")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# --- detect triplet ---
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows")
    elseif(APPLE)
        set(VCPKG_TARGET_TRIPLET "x64-osx")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()
endif()

# --- source files ---
set(SRC
        src/token.c
        src/parser_manager.c
        src/api.c
)

# define target FIRST
add_library(tree_sitter_tokenizer SHARED ${SRC})
target_include_directories(tree_sitter_tokenizer PUBLIC src)

# --- locate and link tree-sitter manually ---
set(TREE_SITTER_INCLUDE "${VCPKG_ROOT}_installed/${VCPKG_TARGET_TRIPLET}/include")
set(TREE_SITTER_LIB_DIR "${VCPKG_ROOT}_installed/${VCPKG_TARGET_TRIPLET}/lib")

find_library(TREE_SITTER_LIB tree-sitter HINTS "${TREE_SITTER_LIB_DIR}" REQUIRED)

# apply to target AFTER it exists
target_include_directories(tree_sitter_tokenizer PRIVATE "${TREE_SITTER_INCLUDE}")
target_link_libraries(tree_sitter_tokenizer PRIVATE "${TREE_SITTER_LIB}")

message(STATUS "Using tree-sitter include path: ${TREE_SITTER_INCLUDE}")
message(STATUS "Using tree-sitter library: ${TREE_SITTER_LIB}")

# --- auto-include grammars ---
# --- auto-include grammars, handling nested src layouts like php/php_only ---
file(GLOB GRAMMAR_DIRS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-*")
foreach(dir ${GRAMMAR_DIRS})
    if(EXISTS "${dir}/src/parser.c")
        message(STATUS "Including grammar: ${dir}/src")
        target_include_directories(tree_sitter_tokenizer PRIVATE "${dir}/src")
        target_sources(tree_sitter_tokenizer PRIVATE "${dir}/src/parser.c")
        if(EXISTS "${dir}/src/scanner.c")
            target_sources(tree_sitter_tokenizer PRIVATE "${dir}/src/scanner.c")
        endif()
    else()
        # print in red
        message("!!! MISSING !!!    ${dir}/src/parser.c")
    endif()
endforeach()

# --- manual grammar overrides ---
# PHP has sources in php_only/src
if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php_only/src/parser.c")
    message(STATUS "Manually including PHP grammar (grammars/tree-sitter-php/php_only/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php_only/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php_only/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php_only/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php_only/src/scanner.c"
        )
    endif()
endif()
# PHP has sources in php/src
if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php/src/parser.c")
    message(STATUS "Manually including PHP grammar (grammars/tree-sitter-php/php/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-php/php/src/scanner.c"
        )
    endif()
endif()
# XML has sources in xml/src
if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/xml/src/parser.c")
    message(STATUS "Manually including XML grammar (grammars/tree-sitter-xml/xml/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/xml/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/xml/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/xml/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/xml/src/scanner.c"
        )
    endif()
endif()
# DTD has sources in dtd/src
if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/dtd/src/parser.c")
    message(STATUS "Manually including XML grammar (grammars/tree-sitter-xml/dtd/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/dtd/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/dtd/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/dtd/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-xml/dtd/src/scanner.c"
        )
    endif()
endif()
# CSV has sources in csv/src
if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/csv/src/parser.c")
    message(STATUS "Manually including CSV grammar (grammars/tree-sitter-csv/csv/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/csv/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/csv/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/csv/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/csv/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including PSV grammar (grammars/tree-sitter-csv/psv/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/psv/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/psv/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/psv/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/psv/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including TSV grammar (grammars/tree-sitter-csv/tsv/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/tsv/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/tsv/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/tsv/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-csv/tsv/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including F# fsharp grammar (grammars/tree-sitter-fsharp/fsharp/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-fsharp/fsharp/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-fsharp/fsharp/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-fsharp/fsharp/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-fsharp/fsharp/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including TypeScript grammar (grammars/tree-sitter-typescript/typescript/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/typescript/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/typescript/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/typescript/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/typescript/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including TSX grammar (grammars/tree-sitter-typescript/tsx/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/tsx/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/tsx/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/tsx/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-typescript/tsx/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including V lang grammar (grammars/tree-sitter-v/tree_sitter_v/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-v/tree_sitter_v/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-v/tree_sitter_v/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-v/tree_sitter_v/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-v/tree_sitter_v/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including OCAML lang grammar (grammars/tree-sitter-ocaml/grammars/ocaml/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/ocaml/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/ocaml/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/ocaml/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/ocaml/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including OCAML interface grammar (grammars/tree-sitter-ocaml/grammars/interface/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/interface/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/interface/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/interface/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/interface/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including OCAML type grammar (grammars/tree-sitter-ocaml/grammars/type/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/type/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/type/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/type/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-ocaml/grammars/type/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including Markdown grammar (grammars/tree-sitter-markdown/tree-sitter-markdown/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including Markdown inline grammar (grammars/tree-sitter-markdown/tree-sitter-markdown-inline/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown-inline/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown-inline/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown-inline/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-markdown/tree-sitter-markdown-inline/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including Problog grammar (grammars/tree-sitter-problog/grammars/problog/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/problog/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/problog/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/problog/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/problog/src/scanner.c"
        )
    endif()

    message(STATUS "Manually including Problog grammar (grammars/tree-sitter-problog/grammars/prolog/src)")
    target_include_directories(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/prolog/src"
    )
    target_sources(tree_sitter_tokenizer PRIVATE
            "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/prolog/src/parser.c"
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/prolog/src/scanner.c")
        target_sources(tree_sitter_tokenizer PRIVATE
                "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-problog/grammars/prolog/src/scanner.c"
        )
    endif()
endif()
