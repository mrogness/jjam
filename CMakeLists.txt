cmake_minimum_required(VERSION 3.20)
project(JJAM LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(JJAM_BUILD_APP "Build the Raspberry Pi application and device backends" ON)
include(CTest)

# The same loop is linked by the application and by hardware-free unit tests.
add_library(hardware_loop src/hardware_loop.cpp)
target_include_directories(hardware_loop PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/tts/include"
)

add_subdirectory(display)

if(JJAM_BUILD_APP)
  find_package(PkgConfig REQUIRED)
  add_subdirectory(camera)
  add_subdirectory(tts)
  pkg_check_modules(TESSERACT REQUIRED IMPORTED_TARGET tesseract)
  pkg_check_modules(LEPTONICA REQUIRED IMPORTED_TARGET lept)

  add_executable(ocr_app src/main.cpp ocr/ocr.cpp)
  target_link_libraries(ocr_app PRIVATE hardware_loop display tts
    PkgConfig::TESSERACT PkgConfig::LEPTONICA)
  add_dependencies(ocr_app camera_hello)
  target_compile_definitions(ocr_app PRIVATE
    CAMERA_EXECUTABLE="$<TARGET_FILE:camera_hello>"
    PIPER_EXECUTABLE_DEFAULT="${PIPER_EXECUTABLE}"
    PIPER_MODEL_DEFAULT="${PIPER_MODEL}"
  )
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
