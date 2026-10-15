cmake_minimum_required(VERSION 3.16)

project(MyProject LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MD4C_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/md4c"
)

add_library(md4c STATIC
    "${MD4C_DIR}/md4c.c"
    "${MD4C_DIR}/md4c.h"
)

target_include_directories(md4c
    PUBLIC
        "${MD4C_DIR}"
)

add_library(md4c-html STATIC
    "${MD4C_DIR}/md4c-html.c"
    "${MD4C_DIR}/md4c-html.h"
    "${MD4C_DIR}/entity.c"
    "${MD4C_DIR}/entity.h"
)

target_include_directories(md4c-html
    PUBLIC
        "${MD4C_DIR}"
)

target_link_libraries(md4c-html
    PUBLIC
        md4c
)

add_executable(ssg
    src/main.cpp
)

target_link_libraries(ssg
    PRIVATE
        md4c-html
)
