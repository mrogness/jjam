pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pngwriter")
find_package(PNGwriter 0.7.0 CONFIG REQUIRED)
find_library(LIBYUV_LIBRARY NAMES yuv REQUIRED)

add_executable(camera_hello hello.cpp PNGImgWrite.cpp)
target_compile_features(camera_hello PRIVATE cxx_std_20)
target_link_libraries(camera_hello PRIVATE
  PkgConfig::LIBCAMERA PNGwriter::PNGwriter ${LIBYUV_LIBRARY})
