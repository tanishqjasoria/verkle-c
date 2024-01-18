cmake_minimum_required(VERSION 3.27)
project(FrE)

set(CMAKE_CXX_STANDARD 23)

add_library(FrE SHARED library.cpp
        library.h)
