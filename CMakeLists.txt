cmake_minimum_required(VERSION 3.10)
project(TesseractExample)

# Find libcamera and add it
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)

set(CMAKE_CXX_STANDARD 17)

add_executable(ocr_app src/main.cpp)
target_link_libraries(ocr_app display)

#add_library(display display/display.cpp)

# Camera Hello
add_executable(camera_hello camera/hello.cpp)
target_compile_features(camera_hello PRIVATE cxx_std_17)
target_link_libraries(camera_hello PRIVATE PkgConfig::LIBCAMERA)