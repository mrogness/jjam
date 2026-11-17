#include <deque>
#include <sstream>
#include <stdexcept>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mocks/mock_loop_services.h"

namespace hardware::test {
namespace {
using namespace testing;

class HardwareLoopTest : public Test {
protected:
  StrictMock<MockLoopServices> mock;
  LoopServices services = mock.callbacks();
  std::deque<std::optional<std::string>> commands;
  std::vector<std::string> statuses;
  std::ostringstream out, errors;
  const Audio page{{"First sentence.", "first.mp3"},
                   {"Second sentence.", "second.mp3"}};

  void SetUp() override {
    EXPECT_CALL(mock, readCommand(_)).WillRepeatedly([this](int) {
      if (commands.empty()) {
        ADD_FAILURE() << "Loop read an unexpected extra command";
        return std::optional<std::string>("q");
      }
      auto command = commands.front();
      commands.pop_front();
      return command;
    });
    EXPECT_CALL(mock, showStatus(_)).WillRepeatedly(
        [this](const std::string &text) { statuses.push_back(text); });
  }

  void run() {
    runHardwareLoop(services, out, errors);
    EXPECT_TRUE(commands.empty());
  }

  void expectPage(const Audio &audio) {
    InSequence sequence;
    EXPECT_CALL(mock, capture()).WillOnce(Return(true));
    EXPECT_CALL(mock, recognize()).WillOnce(Return(true));
    EXPECT_CALL(mock, synthesize(_)).WillOnce(
        [audio](const tts::SentenceReadyCallback &ready) {
          for (std::size_t i = 0; i < audio.size(); ++i)
            ready(i + 1, audio.size(), audio[i]);
          return audio;
        });
  }

  void expectPlayback(const std::vector<std::size_t> &indices,
                      bool finished = false) {
    InSequence sequence;
    for (auto index : indices) {
      EXPECT_CALL(mock, startPlayback(page[index].mp3Path));
      EXPECT_CALL(mock, playbackFinished()).WillOnce(Return(finished));
      EXPECT_CALL(mock, stopPlayback());
    }
  }
};

TEST_F(HardwareLoopTest, QuitDoesNotStartDevices) {
  commands = {"q"};
  run();
  EXPECT_THAT(statuses, ElementsAre("Ready: o + Enter"));
}

TEST_F(HardwareLoopTest, CaptureRunsPipelineAndPlaysAllSentences) {
  commands = {"o", "q"};
  expectPage(page);
  expectPlayback({0, 1}, true);
  run();
  EXPECT_TRUE(errors.str().empty());
  EXPECT_THAT(statuses, ElementsAre(
      "Ready: o + Enter", "Capturing...", "Recognizing...", "Creating speech",
      "TTS 1/2", "TTS 2/2", "Speaking 1/2", "Speaking 2/2", "Ready: o or l"));
}

TEST_F(HardwareLoopTest, CaptureFailureSkipsOcrSpeechAndPlayback) {
  commands = {"o", "q"};
  EXPECT_CALL(mock, capture()).WillOnce(Return(false));
  run();
  EXPECT_THAT(errors.str(), HasSubstr("Camera capture failed"));
  EXPECT_THAT(statuses.back(), Eq("Error: see console"));
}

TEST_F(HardwareLoopTest, OcrFailureSkipsSpeechAndPlayback) {
  commands = {"o", "q"};
  EXPECT_CALL(mock, capture()).WillOnce(Return(true));
  EXPECT_CALL(mock, recognize()).WillOnce(Return(false));
  run();
  EXPECT_THAT(errors.str(), HasSubstr("OCR failed"));
  EXPECT_THAT(statuses.back(), Eq("Error: see console"));
}

TEST_F(HardwareLoopTest, SpeechFailureReportsErrorAndReturnsToCommands) {
  commands = {"o", "p", "q"};
  EXPECT_CALL(mock, capture()).WillOnce(Return(true));
  EXPECT_CALL(mock, recognize()).WillOnce(Return(true));
  EXPECT_CALL(mock, synthesize(_)).WillOnce(Throw(std::runtime_error("TTS failed")));
  EXPECT_CALL(mock, showPreview());
  run();
  EXPECT_THAT(errors.str(), HasSubstr("TTS failed"));
}

TEST_F(HardwareLoopTest, ReplayWithoutPageDoesNotStartPlayback) {
  commands = {"l", "q"};
  run();
  EXPECT_THAT(statuses.back(), Eq("No text found"));
}

TEST_F(HardwareLoopTest, EmptyRecognitionResultDoesNotStartPlayback) {
  commands = {"o", "q"};
  expectPage({});
  run();
  EXPECT_THAT(statuses.back(), Eq("No text found"));
}

TEST_F(HardwareLoopTest, ReplayUsesSavedAudioWithoutRunningPipelineAgain) {
  commands = {"o", "s", "l", "s", "q"};
  expectPage(page);
  expectPlayback({0, 0});
  run();
}

TEST_F(HardwareLoopTest, FailedNewCaptureInvalidatesPreviousAudio) {
  commands = {"o", "s", "o", "l", "q"};
  EXPECT_CALL(mock, capture()).WillOnce(Return(true)).WillOnce(Return(false));
  EXPECT_CALL(mock, recognize()).WillOnce(Return(true));
  EXPECT_CALL(mock, synthesize(_)).WillOnce(Return(page));
  expectPlayback({0});
  run();
  EXPECT_THAT(statuses.back(), Eq("No text found"));
}

TEST_F(HardwareLoopTest, FailedNewSynthesisInvalidatesPreviousAudio) {
  commands = {"o", "s", "o", "l", "q"};
  EXPECT_CALL(mock, capture()).Times(2).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, recognize()).Times(2).WillRepeatedly(Return(true));
  EXPECT_CALL(mock, synthesize(_)).WillOnce(Return(page))
      .WillOnce(Throw(std::runtime_error("TTS failed")));
  expectPlayback({0});
  run();
  EXPECT_THAT(statuses.back(), Eq("No text found"));
}

TEST_F(HardwareLoopTest, PreviewDoesNotStartPipeline) {
  commands = {"p", "q"};
  EXPECT_CALL(mock, showPreview());
  run();
}

TEST_F(HardwareLoopTest, IncompleteAndUnknownCommandsDoNotStartDevices) {
  commands = {std::nullopt, "", "unknown", "q"};
  run();
  EXPECT_THAT(out.str(), HasSubstr("Use o, l, p, or q"));
}

TEST_F(HardwareLoopTest, PlaybackExceptionStopsPlayerAndAllowsRecovery) {
  commands = {"o", "p", "q"};
  expectPage(page);
  {
    InSequence sequence;
    EXPECT_CALL(mock, startPlayback(page[0].mp3Path));
    EXPECT_CALL(mock, playbackFinished())
        .WillOnce(Throw(std::runtime_error("Playback failed")));
    EXPECT_CALL(mock, stopPlayback());
    EXPECT_CALL(mock, showPreview());
  }
  run();
  EXPECT_THAT(errors.str(), HasSubstr("Playback failed"));
}

TEST_F(HardwareLoopTest, PlayerStartFailureDoesNotStopAnUnstartedPlayer) {
  commands = {"o", "q"};
  expectPage(page);
  EXPECT_CALL(mock, startPlayback(page[0].mp3Path))
      .WillOnce(Throw(std::runtime_error("Cannot start player")));
  run();
  EXPECT_THAT(errors.str(), HasSubstr("Cannot start player"));
}

struct NavigationCase {
  const char *name;
  std::vector<std::string> commands;
  std::vector<std::size_t> played;
};

void PrintTo(const NavigationCase &value, std::ostream *stream) {
  *stream << value.name;
}

class PlaybackNavigationTest : public HardwareLoopTest,
                               public WithParamInterface<NavigationCase> {};

TEST_P(PlaybackNavigationTest, StopsOldPlayerBeforeNavigating) {
  for (const auto &command : GetParam().commands) commands.push_back(command);
  expectPage(page);
  expectPlayback(GetParam().played);
  run();
  EXPECT_TRUE(errors.str().empty());
}

INSTANTIATE_TEST_SUITE_P(Commands, PlaybackNavigationTest, Values(
    NavigationCase{"Next", {"o", "n", "s", "q"}, {0, 1}},
    NavigationCase{"Previous", {"o", "n", "b", "s", "q"}, {0, 1, 0}},
    NavigationCase{"PreviousAtFirst", {"o", "b", "s", "q"}, {0, 0}},
    NavigationCase{"Restart", {"o", "r", "s", "q"}, {0, 0}},
    NavigationCase{"SkipLast", {"o", "n", "n", "q"}, {0, 1}},
    NavigationCase{"Stop", {"o", "s", "q"}, {0}},
    NavigationCase{"Quit", {"o", "q"}, {0}}
), [](const TestParamInfo<NavigationCase> &info) { return info.param.name; });

} // namespace
} // namespace hardware::test
