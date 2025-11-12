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
set(TREE_SITTER_INCLUDE "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
set(TREE_SITTER_LIB_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")

find_library(TREE_SITTER_LIB tree-sitter HINTS "${TREE_SITTER_LIB_DIR}" REQUIRED)

# apply to target AFTER it exists
target_include_directories(tree_sitter_tokenizer PRIVATE "${TREE_SITTER_INCLUDE}")
target_link_libraries(tree_sitter_tokenizer PRIVATE "${TREE_SITTER_LIB}")

message(STATUS "Using tree-sitter include path: ${TREE_SITTER_INCLUDE}")
message(STATUS "Using tree-sitter library: ${TREE_SITTER_LIB}")

# --- auto-include grammars ---
file(GLOB GRAMMAR_DIRS "${CMAKE_SOURCE_DIR}/grammars/tree-sitter-*")
foreach(dir ${GRAMMAR_DIRS})
    if(EXISTS "${dir}/src/parser.c")
        message(STATUS "Including grammar: ${dir}")
        target_include_directories(tree_sitter_tokenizer PRIVATE "${dir}/src")
        target_sources(tree_sitter_tokenizer PRIVATE "${dir}/src/parser.c")
        if(EXISTS "${dir}/src/scanner.c")
            target_sources(tree_sitter_tokenizer PRIVATE "${dir}/src/scanner.c")
        endif()
    endif()
endforeach()
