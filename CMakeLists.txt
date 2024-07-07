cmake_minimum_required(VERSION 3.13)
project(fm_radio)

file(GLOB_RECURSE SRC "src/*.cpp")

include(${SDRPP_MODULE_CMAKE})

target_include_directories(fm_radio PRIVATE "src/")