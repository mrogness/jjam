// One page at a time: libcamera capture, OCR, sentence synthesis, playback.
#include <cerrno>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hardware_loop.h"
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

} // namespace

int main() {
  display::Display screen("Main Display", true);
  const fs::path imagePath = "capture.png";
  const fs::path textPath = "ocr/text/text_out.txt";
  const fs::path outputDir = "output";
  std::unique_ptr<tts::PiperTTS> piper;
  std::unique_ptr<Player> player;
  hardware::LoopServices services{
      readCommand,
      [&] {
        // Validate the voice and prepare output before starting a new capture.
        piper = std::make_unique<tts::PiperTTS>(PIPER_EXECUTABLE_DEFAULT,
                                               PIPER_MODEL_DEFAULT);
        fs::create_directories(textPath.parent_path());
        return captureImage(imagePath);
      },
      [&] { return runOCR(imagePath.string(), textPath.string()); },
      [&](const tts::SentenceReadyCallback &onReady) {
        return tts::textFileToMp3s(textPath, outputDir, *piper, onReady);
      },
      [&](const fs::path &path) { player = std::make_unique<Player>(path); },
      [&] { return player->finished(); },
      [&] { player.reset(); },
      [&](const std::string &text) { screen.show_status(text); },
      [&] { screen.show_preview_feed(); }
  };
  hardware::runHardwareLoop(services, std::cout, std::cerr);
  return 0;
}
