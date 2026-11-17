#pragma once

#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "tts/tts_pipeline.h"

namespace hardware {

using Audio = std::vector<tts::SentenceAudio>;

// main.cpp supplies device operations; tests supply mocks with the same contract.
struct LoopServices {
  std::function<std::optional<std::string>(int timeoutMs)> readCommand;
  std::function<bool()> capture;
  std::function<bool()> recognize;
  std::function<Audio(const tts::SentenceReadyCallback &)> synthesize;
  std::function<void(const std::filesystem::path &)> startPlayback;
  std::function<bool()> playbackFinished;
  // Must not throw. Called once for every successfully started player.
  std::function<void()> stopPlayback;
  std::function<void(const std::string &)> showStatus;
  std::function<void()> showPreview;
};

void runHardwareLoop(LoopServices &services, std::ostream &out,
                     std::ostream &errors);

} // namespace hardware
