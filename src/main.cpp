// One page at a time: libcamera capture, OCR, sentence synthesis, playback.
#include <cerrno>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../display/display.h"
#include "../ocr/ocr.h"
#include "tts/tts_pipeline.h"

extern char **environ;
namespace fs = std::filesystem;

namespace {
// Pass arguments directly; paths and OCR text are never shell commands.
pid_t startProcess(std::vector<std::string> arguments) {
  std::vector<char *> argv;
  for (auto &argument : arguments) argv.push_back(argument.data());
  argv.push_back(nullptr);
  pid_t child;
  const int error = posix_spawnp(&child, argv[0], nullptr, nullptr,
                                argv.data(), environ);
  if (error != 0)
    throw std::runtime_error("Cannot start " + arguments[0] +
                             " (error " + std::to_string(error) + ")");
  return child;
}

bool waitSuccess(pid_t child) {
  int status;
  pid_t result;
  do { result = waitpid(child, &status, 0); }
  while (result == -1 && errno == EINTR);
  return result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

// Used for both the idle loop and playback, avoiding mixed buffered stdin reads.
// Commands require Enter. nullopt means no complete command yet; EOF quits.
std::optional<std::string> readCommand(int timeoutMs) {
  static std::string pending;
  pollfd input{STDIN_FILENO, POLLIN, 0};
  int result;
  do { result = poll(&input, 1, timeoutMs); }
  while (result == -1 && errno == EINTR);
  if (result == -1) throw std::runtime_error("Cannot poll keyboard input");
  if (result == 0) return std::nullopt;
  char ch;
  ssize_t count;
  do { count = read(STDIN_FILENO, &ch, 1); }
  while (count == -1 && errno == EINTR);
  if (count == 0) return "q";
  if (count == -1) throw std::runtime_error("Cannot read keyboard input");
  if (ch == '\n') {
    std::string command = pending;
    pending.clear();
    return command;
  }
  if (ch != '\r') pending += ch;
  return std::nullopt;
}

bool captureImage(const fs::path &imagePath) {
  // camera_hello writes capture.png in the current working directory.
  fs::remove(imagePath);
  return waitSuccess(startProcess({CAMERA_EXECUTABLE})) &&
         fs::is_regular_file(imagePath) && fs::file_size(imagePath) > 0;
}

// Own exactly one player. Always stop/reap it on navigation or exceptions.
class Player {
public:
  explicit Player(const fs::path &path)
      : child_(startProcess({"mpg123", "-q", path.string()})) {}
  ~Player() {
    if (child_ > 0) {
      kill(child_, SIGKILL);
      int status;
      while (waitpid(child_, &status, 0) == -1 && errno == EINTR) {}
    }
  }
  bool finished() {
    int status;
    pid_t result = waitpid(child_, &status, WNOHANG);
    if (result == 0 || (result == -1 && errno == EINTR)) return false;
    child_ = -1;
    if (result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
      throw std::runtime_error("mpg123 playback failed; check the audio output");
    return true;
  }
  Player(const Player &) = delete;
  Player &operator=(const Player &) = delete;
private:
  pid_t child_;
};

// false requests application exit; true returns to the idle loop.
bool playSentences(const std::vector<tts::SentenceAudio> &audio,
                   display::Display &screen) {
  if (audio.empty()) {
    screen.show_status("No text found");
    return true;
  }
  std::cout << "Playback: n=next, b=previous, r=restart sentence, "
               "s=stop, q=quit (then Enter).\n";
  for (std::size_t i = 0; i < audio.size();) {
    screen.show_status("Speaking " + std::to_string(i + 1) + "/" +
                       std::to_string(audio.size()));
    std::cout << "[" << i + 1 << "/" << audio.size() << "] "
              << audio[i].sentence << std::endl;
    Player player(audio[i].mp3Path);
    for (;;) {
      if (player.finished()) { ++i; break; }
      const auto command = readCommand(100);
      if (!command) continue;
      if (*command == "q") return false;
      if (*command == "s") return true;
      if (*command == "n") { ++i; break; }
      if (*command == "b") { if (i > 0) --i; break; }
      if (*command == "r") break;
    }
    // Player destruction stops the old sentence before starting another.
  }
  return true;
}
} // namespace

int main() {
  display::Display screen("Main Display", true);
  const fs::path imagePath = "capture.png";
  const fs::path textPath = "ocr/text/text_out.txt";
  const fs::path outputDir = "output";
  std::vector<tts::SentenceAudio> audio;
  bool running = true;
  std::cout << "Run from the repository root. Commands require Enter.\n"
               "o=capture/read/speak, l=replay, p=preview status, q=quit\n";
  screen.show_status("Ready: o + Enter");
  while (running) {
    try {
      const auto command = readCommand(-1);
      if (!command) continue;
      if (*command == "q") break;
      if (*command == "p") { screen.show_preview_feed(); continue; }
      if (*command == "o") {
        // Invalidate the previous page even if capture or synthesis fails.
        audio.clear();
        const tts::PiperTTS piper(PIPER_EXECUTABLE_DEFAULT, PIPER_MODEL_DEFAULT);
        fs::create_directories(textPath.parent_path());
        screen.show_status("Capturing...");
        if (!captureImage(imagePath)) throw std::runtime_error("Camera capture failed");
        screen.show_status("Recognizing...");
        if (!runOCR(imagePath.string(), textPath.string()))
          throw std::runtime_error("OCR failed");
        screen.show_status("Creating speech");
        // Finish synthesis first, so next/previous can access every sentence.
        audio = tts::textFileToMp3s(
            textPath, outputDir, piper,
            [&](std::size_t i, std::size_t total, const tts::SentenceAudio &) {
              screen.show_status("TTS " + std::to_string(i) + "/" +
                                 std::to_string(total));
            });
        running = playSentences(audio, screen);
      } else if (*command == "l") {
        running = playSentences(audio, screen);
      } else {
        std::cout << "Use o, l, p, or q, then Enter.\n";
      }
      if (running && !audio.empty()) screen.show_status("Ready: o or l");
    } catch (const std::exception &error) {
      std::cerr << "Error: " << error.what() << '\n';
      screen.show_status("Error: see console");
    }
  }
  return 0;
}
