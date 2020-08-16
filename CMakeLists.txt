cmake_minimum_required(VERSION 3.17)
project(tlisp C)

set(CMAKE_C_STANDARD 11)

add_executable(tlisp main.c mpc.c)
target_link_libraries(tlisp LINK_PUBLIC readline)