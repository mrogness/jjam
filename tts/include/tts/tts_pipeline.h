#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "tts/piper_tts.h"

namespace tts {

struct SentenceAudio {
    std::string sentence;          // the text that was spoken
    std::filesystem::path mp3Path; // e.g. output/sentence_001.mp3
};

// Called after each MP3 is finished. index starts at 1.
// Useful for starting playback of sentence 1 while the rest are generated.
using SentenceReadyCallback =
    std::function<void(std::size_t index, std::size_t total, const SentenceAudio&)>;

// Reads an OCR text file, splits it into sentences, and writes
//   outputDir/sentence_001.mp3, sentence_002.mp3, ...
//   outputDir/index.txt   (one line per sentence: "<file name><TAB><sentence>")
//
// Old sentence_*.mp3 files and index.txt in outputDir are removed first, so
// each new page starts clean. Throws std::runtime_error on failure.
std::vector<SentenceAudio> textFileToMp3s(const std::filesystem::path& textFile,
                                          const std::filesystem::path& outputDir,
                                          const PiperTTS& piper,
                                          const SentenceReadyCallback& onReady = {});

// Reads a whole text file into a string. Throws std::runtime_error on failure.
std::string readTextFile(const std::filesystem::path& path);

}  // namespace tts
