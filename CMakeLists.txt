cmake_minimum_required(VERSION 3.10)
project(TesseractExample)

set(CMAKE_CXX_STANDARD 17)

find_package(Tesseract CONFIG REQUIRED)


add_executable(tesseract_test
    ocr/tesseract_test.cpp
    ocr/ocr.cpp
)

target_link_libraries(tesseract_test PRIVATE
    Tesseract::libtesseract
)