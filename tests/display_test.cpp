#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "display/display.h"
#include "mocks/mock_waveshare.h"

namespace display::test {
namespace {
using namespace testing;

class DisplayTest : public Test {
protected:
  StrictMock<MockWaveshare> mock;

  void SetUp() override { waveshare = &mock; }
  void TearDown() override { waveshare = nullptr; }

  void expectInitialization() {
    {
      InSequence sequence;
      EXPECT_CALL(mock, moduleInit()).WillOnce(Return(0));
      EXPECT_CALL(mock, lcdInit());
      EXPECT_CALL(mock, lcdClear(WHITE));
      EXPECT_CALL(mock, backlight(_));
      EXPECT_CALL(mock, newImage(NotNull(), 240, 320, 90, WHITE, 16));
      EXPECT_CALL(mock, rotate(ROTATE_270));
      EXPECT_CALL(mock, selectImage(NotNull()));
      EXPECT_CALL(mock, clear(WHITE));
      EXPECT_CALL(mock, display(NotNull()));
    }
    EXPECT_CALL(mock, moduleExit());
  }

  void expectRender(const std::string &text, UWORD background, UWORD foreground) {
    InSequence sequence;
    EXPECT_CALL(mock, selectImage(NotNull()));
    EXPECT_CALL(mock, clear(background));
    EXPECT_CALL(mock, drawText(_, _, StrEq(text), &Font20, foreground, background))
        .WillOnce([](UWORD x, UWORD y, const char *, sFONT *font, UWORD, UWORD) {
          EXPECT_LT(x, LCD_2IN_WIDTH);
          EXPECT_LE(y + font->Height, LCD_2IN_HEIGHT);
        });
    EXPECT_CALL(mock, display(NotNull()));
  }
};

TEST_F(DisplayTest, DisabledDisplayNeverTouchesHardware) {
  Display screen("test", false);
  screen.show_status("Ready");
  screen.show_tts_status();
  screen.show_ocr_status();
  screen.show_preview_feed();
}

TEST_F(DisplayTest, FailedInitializationSkipsRenderingAndShutdown) {
  EXPECT_CALL(mock, moduleInit()).WillOnce(Return(1));
  Display screen("test", true);
  screen.show_status("Ready");
  screen.show_tts_status();
  screen.show_ocr_status();
  screen.show_preview_feed();
}

TEST_F(DisplayTest, SuccessfulInitializationFlushesAndShutsDownOnce) {
  expectInitialization();
  Display screen("test", true);
}

TEST_F(DisplayTest, StatusDrawsRequestedTextAndFlushes) {
  expectInitialization();
  Display screen("test", true);
  expectRender("Recognizing...", BLUE, WHITE);
  screen.show_status("Recognizing...");
}

TEST_F(DisplayTest, TtsStatusUsesBlueAndWhite) {
  expectInitialization();
  Display screen("test", true);
  expectRender("TTS READY", BLUE, WHITE);
  EXPECT_EQ(screen.show_tts_status(), "TTS status displayed");
}

TEST_F(DisplayTest, OcrStatusUsesGreenAndBlack) {
  expectInitialization();
  Display screen("test", true);
  expectRender("OCR READY", GREEN, BLACK);
  EXPECT_EQ(screen.show_ocr_status(), "OCR status displayed");
}

TEST_F(DisplayTest, PreviewStatusUsesMagentaAndWhite) {
  expectInitialization();
  Display screen("test", true);
  expectRender("PREVIEW", MAGENTA, WHITE);
  EXPECT_EQ(screen.show_preview_feed(), "Preview feed displayed");
}

TEST_F(DisplayTest, LongTextStartsAtLeftEdgeWithoutUnsignedUnderflow) {
  expectInitialization();
  Display screen("test", true);
  const std::string text(100, 'A');
  EXPECT_CALL(mock, selectImage(NotNull()));
  EXPECT_CALL(mock, clear(BLUE));
  EXPECT_CALL(mock, drawText(0, _, StrEq(text), &Font20, WHITE, BLUE));
  EXPECT_CALL(mock, display(NotNull()));
  screen.show_status(text);
}

} // namespace
} // namespace display::test
