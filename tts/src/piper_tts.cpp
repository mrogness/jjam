#include "tts/piper_tts.h"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <sys/wait.h>  // WIFEXITED / WEXITSTATUS (Linux / Raspberry Pi)

namespace fs = std::filesystem;

namespace tts {

namespace {

// Wraps a string in single quotes so the shell treats it as one argument,
// even if it contains spaces or special characters.
std::string shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

}  // namespace

PiperTTS::PiperTTS(std::string piperExecutable, fs::path modelPath)
    : piperExecutable_(std::move(piperExecutable)), modelPath_(std::move(modelPath)) {
    if (modelPath_.empty()) {
        throw std::runtime_error(
            "No Piper voice model given. Pass --model <voice.onnx> or configure "
            "CMake with -DPIPER_MODEL=<voice.onnx>");
    }
    if (!fs::exists(modelPath_)) {
        throw std::runtime_error("Piper voice model not found: " + modelPath_.string());
    }
    fs::path config = modelPath_;
    config += ".json";
    if (!fs::exists(config)) {
        throw std::runtime_error("Voice config not found: " + config.string() +
                                 " (download it together with the .onnx file)");
    }
}

void PiperTTS::synthesizeToWav(const std::string& text, const fs::path& wavPath) const {
    // The text goes to Piper through a file on stdin, never on the command
    // line, so OCR text can't break (or inject into) the shell command.
    fs::path textPath = wavPath;
    textPath.replace_extension(".txt");
    {
        std::ofstream out(textPath);
        if (!out) throw std::runtime_error("Could not write " + textPath.string());
        out << text << '\n';
    }

    std::error_code ec;
    fs::remove(wavPath, ec);

    const std::string command = shellQuote(piperExecutable_) +
                                " -m " + shellQuote(modelPath_.string()) +
                                " -f " + shellQuote(wavPath.string()) +
                                " < " + shellQuote(textPath.string()) +
                                " > /dev/null";

    const int status = std::system(command.c_str());
    fs::remove(textPath, ec);

    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Piper failed. Command was:\n  " + command);
    }
    if (!fs::exists(wavPath) || fs::file_size(wavPath) == 0) {
        throw std::runtime_error("Piper did not produce audio for: " + text);
    }
}

}  // namespace tts
