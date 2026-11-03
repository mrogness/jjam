#pragma once

#include <filesystem>
#include <string>

namespace tts {

// Runs the Piper command-line program to turn text into a WAV file.
class PiperTTS {
public:
    // piperExecutable: "piper" (if it's on the PATH) or a full path to it.
    // modelPath:       the .onnx voice model (its .onnx.json must sit next to it).
    PiperTTS(std::string piperExecutable, std::filesystem::path modelPath);

    // Speaks `text` into `wavPath`. Throws std::runtime_error on failure.
    void synthesizeToWav(const std::string& text,
                         const std::filesystem::path& wavPath) const;

private:
    std::string piperExecutable_;
    std::filesystem::path modelPath_;
};

}  // namespace tts
