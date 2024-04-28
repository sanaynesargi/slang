cmake_minimum_required(VERSION 3.20)

project(slang)

set(CMAKE_CXX_STANDARD 20)

add_executable(sl src/main.cpp
        src/main.cpp
        src/tokenization.hpp
        src/parser.hpp
        src/generation.hpp)
