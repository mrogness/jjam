include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# GoogleTest v1.17.0, pinned so every developer and CI use the same sources.
FetchContent_Declare(googletest
  URL https://codeload.github.com/google/googletest/zip/52eb8108c5bdec04579160ae17225d66034bd723
  URL_HASH SHA256=9a56a54ae784394ff664cd55e8f4c9a03b503ebf0cb99576321c78ab3d87ca84
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Partners can add another suite with jjam_add_test(name sources...), then link
# its production library with target_link_libraries(name PRIVATE library).
function(jjam_add_test target)
  add_executable(${target} ${ARGN})
  target_link_libraries(${target} PRIVATE GTest::gmock_main)
  gtest_discover_tests(${target} PROPERTIES TIMEOUT 5)
endfunction()

jjam_add_test(hardware_loop_tests hardware_loop_test.cpp)
target_link_libraries(hardware_loop_tests PRIVATE hardware_loop)

# Compile the real wrapper, but replace vendor calls rather than linking GPIO.
jjam_add_test(display_tests display_test.cpp mocks/waveshare.cpp
  "${PROJECT_SOURCE_DIR}/display/DisplayWaveshare.cpp"
  "${WAVESHARE_C_ROOT}/lib/Fonts/font20.c")
target_include_directories(display_tests PRIVATE "${PROJECT_SOURCE_DIR}")
target_link_libraries(display_tests PRIVATE waveshare_headers)
