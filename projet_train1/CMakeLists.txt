cmake_minimum_required(VERSION 3.9)
project(projet_train1 C)

set(CMAKE_C_STANDARD 11)

add_executable(projet_train1 main.c header.h)
target_compile_options(projet_train1 PRIVATE "-DDEBUG" "-DTEST" "-Wall" "-Wextra" "-Werror" "-O3")