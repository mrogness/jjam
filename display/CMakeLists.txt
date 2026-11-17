set(WAVESHARE_C_ROOT
  "${PROJECT_SOURCE_DIR}/third_party/LCD_Module_RPI_code/RaspberryPi/c"
  CACHE PATH "Path to Waveshare RaspberryPi/c directory")

add_library(waveshare_headers INTERFACE)
target_include_directories(waveshare_headers INTERFACE
  "${WAVESHARE_C_ROOT}/lib/Config"
  "${WAVESHARE_C_ROOT}/lib/LCD"
  "${WAVESHARE_C_ROOT}/lib/GUI"
  "${WAVESHARE_C_ROOT}/lib/Fonts"
)

if(JJAM_BUILD_APP)
  find_library(LGPIO_LIBRARY NAMES lgpio REQUIRED)
  find_package(Threads REQUIRED)
  add_library(display
    DisplayWaveshare.cpp
    "${WAVESHARE_C_ROOT}/lib/Config/DEV_Config.c"
    "${WAVESHARE_C_ROOT}/lib/LCD/LCD_2inch.c"
    "${WAVESHARE_C_ROOT}/lib/GUI/GUI_Paint.c"
    "${WAVESHARE_C_ROOT}/lib/GUI/GUI_BMP.c"
    "${WAVESHARE_C_ROOT}/lib/Fonts/font20.c"
  )
  target_compile_definitions(display PRIVATE USE_DEV_LIB)
  target_link_libraries(display PRIVATE waveshare_headers
    ${LGPIO_LIBRARY} Threads::Threads m)
endif()
