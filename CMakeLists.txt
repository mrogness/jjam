cmake_minimum_required(VERSION 3.10)
project(TesseractExample)

# Find libcamera and add it
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Camera Hello
add_executable(camera_hello camera/hello.cpp)
target_compile_features(camera_hello PRIVATE cxx_std_17)

# PNGwriter
list(PREPEND CMAKE_PREFIX_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/camera/pngwriter"
)
find_package(PNGwriter 0.7.0 CONFIG)
target_link_libraries(camera_hello PRIVATE PkgConfig::LIBCAMERA PNGwriter::PNGwriter)

# set(WAVESHARE_C_ROOT "${CMAKE_SOURCE_DIR}/third_party/LCD_Module_RPI_code/RaspberryPi/c"
#     CACHE PATH "Path to Waveshare RaspberryPi/c directory")

# add_executable(ocr_app src/main.cpp)
# target_link_libraries(ocr_app display)

# # Using the C library provided by Waveshare for their display
# set(_ws_required_files
#   "${WAVESHARE_C_ROOT}/lib/Config/DEV_Config.h"
#   "${WAVESHARE_C_ROOT}/lib/LCD/LCD_2inch.h"
#   "${WAVESHARE_C_ROOT}/lib/GUI/GUI_Paint.h"
# )
# foreach(_f IN LISTS _ws_required_files)
#   if(NOT EXISTS "${_f}")
#     message(FATAL_ERROR
#       "Waveshare sources were not found.\n"
#       "Expected: ${_f}\n")
#   endif()
# endforeach()

# set(WAVESHARE_CONFIG_SOURCES
#   "${WAVESHARE_C_ROOT}/lib/Config/DEV_Config.c"
# )
# set(WAVESHARE_LCD_SOURCES
#   "${WAVESHARE_C_ROOT}/lib/LCD/LCD_2inch.c"
# )
# set(WAVESHARE_GUI_SOURCES
#   "${WAVESHARE_C_ROOT}/lib/GUI/GUI_Paint.c"
#   "${WAVESHARE_C_ROOT}/lib/GUI/GUI_BMP.c"
# )
# # Required because DisplayWaveshare.cpp uses Font20, 
# # if wanting other font styles we need to include them here
# set(WAVESHARE_FONT_SOURCES
#   "${WAVESHARE_C_ROOT}/lib/Fonts/font20.c"
# )

# add_library(display
#   display/DisplayWaveshare.cpp
#   ${WAVESHARE_CONFIG_SOURCES}
#   ${WAVESHARE_LCD_SOURCES}
#   ${WAVESHARE_GUI_SOURCES}
#   ${WAVESHARE_FONT_SOURCES}
# )

# target_include_directories(display PRIVATE
#   "${WAVESHARE_C_ROOT}/lib/Config"
#   "${WAVESHARE_C_ROOT}/lib/LCD"
#   "${WAVESHARE_C_ROOT}/lib/GUI"
#   "${WAVESHARE_C_ROOT}/lib/Fonts"
# )

# target_compile_definitions(display PRIVATE USE_DEV_LIB)
# find_library(LGPIO_LIBRARY NAMES lgpio)
# if(NOT LGPIO_LIBRARY)
#   message(FATAL_ERROR
#     "Waveshare DEV backend requires liblgpio.\n"
#     "Install the development package (e.g. liblgpio-dev) and retry.")
# endif()
# target_link_libraries(display PRIVATE ${LGPIO_LIBRARY} pthread m)

# message(STATUS "Display backend: Waveshare C library (DEV/lgpio)")


# # Find libcamera and add it
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)

# set(CMAKE_CXX_STANDARD 17)

# add_executable(ocr_app src/main.cpp)

# End of CMakeLists.txt
# To build the project, run:

# cmake -S . -B build \
#   -DWAVESHARE_C_ROOT=/home/jjam/jjam/third_party/LCD_Module_RPI_code/RaspberryPi/c \
#   -DWAVESHARE_DRIVER_LIB=DEV

# cmake --build build -j

# sudo ./build/ocr_app
    