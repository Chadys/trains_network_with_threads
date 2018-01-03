cmake_minimum_required(VERSION 3.9)
project(projet_train3 C)

set(CMAKE_C_STANDARD 11)

add_executable(projet_train3 main.c header.h)
target_compile_options(projet_train3 PRIVATE "-DDEBUG" "-Wall" "-Wextra" "-Werror" "-O3")