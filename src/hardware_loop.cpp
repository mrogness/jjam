#include "hardware_loop.h"

#include <ostream>
#include <stdexcept>

namespace hardware {
namespace {

// Stop the current sentence on navigation, quit, or an exception.
class Playback {
public:
  Playback(LoopServices &services, const std::filesystem::path &path)
      : services_(services) { services_.startPlayback(path); }
  ~Playback() { services_.stopPlayback(); }
  Playback(const Playback &) = delete;
  Playback &operator=(const Playback &) = delete;
private:
  LoopServices &services_;
};

bool playSentences(const Audio &audio, LoopServices &services,
                   std::ostream &out) {
  if (audio.empty()) {
    services.showStatus("No text found");
    return true;
  }
  out << "Playback: n=next, b=previous, r=restart sentence, "
         "s=stop, q=quit (then Enter).\n";
  for (std::size_t i = 0; i < audio.size();) {
    services.showStatus("Speaking " + std::to_string(i + 1) + "/" +
                        std::to_string(audio.size()));
    out << "[" << i + 1 << "/" << audio.size() << "] "
        << audio[i].sentence << std::endl;
    Playback player(services, audio[i].mp3Path);
    for (;;) {
      if (services.playbackFinished()) { ++i; break; }
      const auto command = services.readCommand(100);
      if (!command) continue;
      if (*command == "q") return false;
      if (*command == "s") return true;
      if (*command == "n") { ++i; break; }
      if (*command == "b") { if (i > 0) --i; break; }
      if (*command == "r") break;
    }
  }
  return true;
}
} // namespace

void runHardwareLoop(LoopServices &services, std::ostream &out,
                     std::ostream &errors) {
  Audio audio;
  bool running = true;
  out << "Run from the repository root. Commands require Enter.\n"
         "o=capture/read/speak, l=replay, p=preview status, q=quit\n";
  services.showStatus("Ready: o + Enter");
  while (running) {
    try {
      const auto command = services.readCommand(-1);
      if (!command) continue;
      if (*command == "q") break;
      if (*command == "p") { services.showPreview(); continue; }
      if (*command == "o") {
        // A failed new page must never replay the previous page.
        audio.clear();
        services.showStatus("Capturing...");
        if (!services.capture()) throw std::runtime_error("Camera capture failed");
        services.showStatus("Recognizing...");
        if (!services.recognize()) throw std::runtime_error("OCR failed");
        services.showStatus("Creating speech");
        audio = services.synthesize(
            [&](std::size_t i, std::size_t total, const tts::SentenceAudio &) {
              services.showStatus("TTS " + std::to_string(i) + "/" +
                                  std::to_string(total));
            });
        running = playSentences(audio, services, out);
      } else if (*command == "l") {
        running = playSentences(audio, services, out);
      } else {
        out << "Use o, l, p, or q, then Enter.\n";
      }
      if (running && !audio.empty()) services.showStatus("Ready: o or l");
    } catch (const std::exception &error) {
      errors << "Error: " << error.what() << '\n';
      services.showStatus("Error: see console");
    }
  }
}
} // namespace hardware
