#pragma once

#include <filesystem>

namespace tts {

// Converts a 16-bit PCM WAV file (what Piper produces) into an MP3 file.
// 64 kbps is plenty for speech. Throws std::runtime_error on failure.
void wavToMp3(const std::filesystem::path& wavPath,
              const std::filesystem::path& mp3Path,
              int bitrateKbps = 64);

}  // namespace tts
