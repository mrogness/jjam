# ---------------------------------------------------------------------------
# Text-to-speech module
# Turns OCR text into one MP3 file per sentence, using Piper (speech) and
# LAME (MP3 encoding).
#
# Requirements on the Raspberry Pi:
#   sudo apt install libmp3lame-dev
#   Piper:  pip install piper-tts   (gives you the `piper` command)
# ---------------------------------------------------------------------------

# --- Find LAME (MP3 encoder library) ---
find_library(MP3LAME_LIBRARY NAMES mp3lame)
find_path(MP3LAME_INCLUDE_DIR NAMES lame/lame.h)
if(NOT MP3LAME_LIBRARY OR NOT MP3LAME_INCLUDE_DIR)
    message(FATAL_ERROR
        "libmp3lame not found. Install it with: sudo apt install libmp3lame-dev")
endif()

# --- Piper settings (override with -DPIPER_MODEL=... when configuring) ---
set(PIPER_EXECUTABLE "piper" CACHE STRING "Piper program name or full path")
set(PIPER_MODEL "" CACHE FILEPATH "Default Piper .onnx voice model")

# --- The TTS library (other parts of the project can link to this) ---
add_library(tts STATIC
    src/sentence_splitter.cpp
    src/piper_tts.cpp
    src/mp3_encoder.cpp
    src/tts_pipeline.cpp
)
target_include_directories(tts
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${MP3LAME_INCLUDE_DIR}
)
target_link_libraries(tts PRIVATE ${MP3LAME_LIBRARY})
target_compile_features(tts PUBLIC cxx_std_17)

# --- Command-line program for testing the TTS on its own ---
add_executable(tts_reader src/main.cpp)
target_link_libraries(tts_reader PRIVATE tts)
target_compile_definitions(tts_reader PRIVATE
    PIPER_EXECUTABLE_DEFAULT="${PIPER_EXECUTABLE}"
    PIPER_MODEL_DEFAULT="${PIPER_MODEL}"
)

# --- Compiler warnings (catch bugs early) ---
foreach(target tts tts_reader)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()
