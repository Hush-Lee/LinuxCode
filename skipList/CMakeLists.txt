cmake_minimum_required(VERSION 3.0)

project(skiplist)

add_executable(skiplist main.cpp)

install(TARGETS skiplist RUNTIME DESTINATION bin)
