// tts_reader: command-line tool for testing the TTS module on its own.
//
//   tts_reader <input.txt> <output_dir> [--model voice.onnx] [--piper path]
//   tts_reader --split-only <input.txt>
//
// The first form writes output_dir/sentence_001.mp3, sentence_002.mp3, ...
// plus output_dir/index.txt. The second just prints the sentences, which is
// handy for checking how OCR text gets split without running Piper.

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "tts/piper_tts.h"
#include "tts/sentence_splitter.h"
#include "tts/tts_pipeline.h"

#ifndef PIPER_EXECUTABLE_DEFAULT
#define PIPER_EXECUTABLE_DEFAULT "piper"
#endif
#ifndef PIPER_MODEL_DEFAULT
#define PIPER_MODEL_DEFAULT ""
#endif

namespace {

void printUsage(const char* program) {
    std::cerr << "Usage:\n"
              << "  " << program << " <input.txt> <output_dir> [--model voice.onnx] [--piper path]\n"
              << "  " << program << " --split-only <input.txt>\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string piperExecutable = PIPER_EXECUTABLE_DEFAULT;
    std::string model = PIPER_MODEL_DEFAULT;
    bool splitOnly = false;
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--model" && i + 1 < argc) {
            model = argv[++i];
        } else if (arg == "--piper" && i + 1 < argc) {
            piperExecutable = argv[++i];
        } else if (arg == "--split-only") {
            splitOnly = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg.rfind("--", 0) == 0) {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            positional.push_back(arg);
        }
    }

    try {
        if (splitOnly) {
            if (positional.size() != 1) {
                printUsage(argv[0]);
                return 1;
            }
            const auto sentences = tts::splitIntoSentences(tts::readTextFile(positional[0]));
            for (std::size_t i = 0; i < sentences.size(); ++i) {
                std::cout << (i + 1) << ": " << sentences[i] << "\n";
            }
            return 0;
        }

        if (positional.size() != 2) {
            printUsage(argv[0]);
            return 1;
        }

        const tts::PiperTTS piper(piperExecutable, model);
        const auto results = tts::textFileToMp3s(
            positional[0], positional[1], piper,
            [](std::size_t index, std::size_t total, const tts::SentenceAudio& audio) {
                std::cout << "[" << index << "/" << total << "] "
                          << audio.mp3Path.filename().string() << "  " << audio.sentence
                          << std::endl;
            });

        std::cout << "Done: " << results.size() << " MP3 file(s) in " << positional[1] << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
