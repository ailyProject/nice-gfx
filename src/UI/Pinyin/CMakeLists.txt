cmake_minimum_required(VERSION 3.15)
project(nice_gfx_pinyin LANGUAGES CXX)

add_library(nice_gfx_pinyin INTERFACE)
target_include_directories(nice_gfx_pinyin INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(nice_gfx_pinyin INTERFACE cxx_std_11)
