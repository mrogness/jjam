#pragma once

#include <gmock/gmock.h>
#include "hardware_loop.h"

namespace hardware::test {

class MockLoopServices {
public:
  MOCK_METHOD(std::optional<std::string>, readCommand, (int));
  MOCK_METHOD(bool, capture, ());
  MOCK_METHOD(bool, recognize, ());
  MOCK_METHOD(Audio, synthesize, (const tts::SentenceReadyCallback &));
  MOCK_METHOD(void, startPlayback, (const std::filesystem::path &));
  MOCK_METHOD(bool, playbackFinished, ());
  MOCK_METHOD(void, stopPlayback, ());
  MOCK_METHOD(void, showStatus, (const std::string &));
  MOCK_METHOD(void, showPreview, ());

  LoopServices callbacks() {
    return {
        [this](int timeout) { return readCommand(timeout); },
        [this] { return capture(); },
        [this] { return recognize(); },
        [this](const tts::SentenceReadyCallback &ready) { return synthesize(ready); },
        [this](const std::filesystem::path &path) { startPlayback(path); },
        [this] { return playbackFinished(); },
        [this] { stopPlayback(); },
        [this](const std::string &text) { showStatus(text); },
        [this] { showPreview(); }
    };
  }
};

} // namespace hardware::test
