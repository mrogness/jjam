#include "tts/tts_pipeline.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "tts/mp3_encoder.h"
#include "tts/sentence_splitter.h"

namespace fs = std::filesystem;

namespace tts {

namespace {

std::string mp3FileName(std::size_t index) {
    std::ostringstream name;
    name << "sentence_" << std::setw(3) << std::setfill('0') << index << ".mp3";
    return name.str();
}

// Removes files from a previous page so old sentences are never played.
void clearOldOutput(const fs::path& dir) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        const bool oldSentence =
            name.rfind("sentence_", 0) == 0 && entry.path().extension() == ".mp3";
        if (oldSentence || name == "index.txt") fs::remove(entry.path());
    }
}

}  // namespace

std::string readTextFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Could not open text file: " + path.string());
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

std::vector<SentenceAudio> textFileToMp3s(const fs::path& textFile,
                                          const fs::path& outputDir,
                                          const PiperTTS& piper,
                                          const SentenceReadyCallback& onReady) {
    const std::vector<std::string> sentences = splitIntoSentences(readTextFile(textFile));

    fs::create_directories(outputDir);
    clearOldOutput(outputDir);

    std::ofstream index(outputDir / "index.txt");
    if (!index) throw std::runtime_error("Could not create index.txt in " + outputDir.string());

    const fs::path tempWav = outputDir / ".tts_temp.wav";
    std::vector<SentenceAudio> results;
    results.reserve(sentences.size());

    try {
        for (std::size_t i = 0; i < sentences.size(); ++i) {
            SentenceAudio audio{sentences[i], outputDir / mp3FileName(i + 1)};

            piper.synthesizeToWav(audio.sentence, tempWav);
            wavToMp3(tempWav, audio.mp3Path);

            index << audio.mp3Path.filename().string() << '\t' << audio.sentence << '\n';
            index.flush();

            results.push_back(audio);
            if (onReady) onReady(i + 1, sentences.size(), results.back());
        }
    } catch (...) {
        std::error_code ec;
        fs::remove(tempWav, ec);
        throw;
    }

    std::error_code ec;
    fs::remove(tempWav, ec);
    return results;
}

}  // namespace tts
