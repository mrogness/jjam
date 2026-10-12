cmake_minimum_required(VERSION 3.10)
project(TesseractExample)

set(CMAKE_CXX_STANDARD 17)

add_executable(ocr_app src/main.cpp)
target_link_libraries(ocr_app display)

add_library(display display/display.cpp)