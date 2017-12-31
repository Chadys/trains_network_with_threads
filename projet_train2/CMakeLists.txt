cmake_minimum_required(VERSION 3.9)
project(projet_train2 C)

set(CMAKE_C_STANDARD 11)

add_executable(projet_train2 main.c header.h)
target_compile_options(projet_train2 PRIVATE "-DDEBUG" "-Wall" "-Wextra" "-Werror" "-g") #TODO replace -g with optimization and add "-lpthread"