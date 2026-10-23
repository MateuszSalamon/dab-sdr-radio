cmake_minimum_required(VERSION 3.10)
project(viterbi_replacement CXX)

add_library(viterbi_replacement STATIC
    viterbi_replacement.cpp
)

target_include_directories(viterbi_replacement
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(viterbi_replacement PUBLIC cxx_std_17)
