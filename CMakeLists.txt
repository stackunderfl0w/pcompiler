cmake_minimum_required(VERSION 3.26)
project(compiler C)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -Wall")


find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

BISON_TARGET(parse src/parse.y ${CMAKE_CURRENT_SOURCE_DIR}/src/parse.c)
FLEX_TARGET(scanner src/scan.l ${CMAKE_CURRENT_SOURCE_DIR}/src/scan.c COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_SOURCE_DIR}/src/scan.h")

add_executable(compiler src/main.c src/scan.c src/parse.c src/hash.c src/ast.c src/dynarray.h src/scope.c src/scope.h)
add_executable(interface src/interface.c src/scan.c src/parse.c src/hash.c src/ast.c src/dynarray.h src/scope.c src/scope.h)

target_link_libraries(compiler LLVM)
target_link_libraries(interface LLVM SDL2 SDL2_ttf)
