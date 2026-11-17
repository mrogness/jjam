#pragma once

#include <gmock/gmock.h>

extern "C" {
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_2inch.h"
}

namespace display::test {

// Only the vendor calls used by our display wrapper need replacements.
class MockWaveshare {
public:
  MOCK_METHOD(UBYTE, moduleInit, ());
  MOCK_METHOD(void, moduleExit, ());
  MOCK_METHOD(void, backlight, (UWORD));
  MOCK_METHOD(void, lcdInit, ());
  MOCK_METHOD(void, lcdClear, (UWORD));
  MOCK_METHOD(void, display, (UBYTE *));
  MOCK_METHOD(void, newImage, (UWORD *, UWORD, UWORD, UWORD, UWORD, UWORD));
  MOCK_METHOD(void, rotate, (UWORD));
  MOCK_METHOD(void, selectImage, (UWORD *));
  MOCK_METHOD(void, clear, (UWORD));
  MOCK_METHOD(void, drawText,
              (UWORD, UWORD, const char *, sFONT *, UWORD, UWORD));
};

// Set by the test fixture; no replacement is linked into the real application.
extern MockWaveshare *waveshare;

} // namespace display::test
