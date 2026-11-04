// Waveshare C library wrapper backend for the Display interface.
#include "display.h"

#include <algorithm>
#include <iostream>
#include <utility>

extern "C" {
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_2inch.h"
}

namespace display {

Display::Display(std::string display_name, bool initialized)
    : display_initialized(false), display_name(std::move(display_name)),
      width(LCD_2IN_WIDTH), height(LCD_2IN_HEIGHT),
      frame_buffer(static_cast<std::size_t>(LCD_2IN_WIDTH) *
                       static_cast<std::size_t>(LCD_2IN_HEIGHT),
                   WHITE) {
  if (!initialized) {
    return;
  }

  display_initialized = initialize_spi_display();
  if (!display_initialized) {
    std::cerr << "Display init failed for " << this->display_name
              << "; running without LCD output.\n";
  }
}

Display::~Display() { shutdown_spi_display(); }

bool Display::initialize_spi_display() {
  if (DEV_ModuleInit() != 0) {
    std::cerr << "Waveshare DEV_ModuleInit failed.\n";
    return false;
  }

  LCD_2IN_Init();
  LCD_2IN_Clear(WHITE);
  LCD_SetBacklight(1010);

  Paint_NewImage(reinterpret_cast<UWORD *>(frame_buffer.data()), LCD_2IN_WIDTH,
                 LCD_2IN_HEIGHT, 90, WHITE, 16);
  Paint_SetRotate(ROTATE_270);
  Paint_SelectImage(reinterpret_cast<UWORD *>(frame_buffer.data()));
  Paint_Clear(WHITE);
  flush();
  return true;
}

void Display::shutdown_spi_display() {
  if (display_initialized) {
    DEV_ModuleExit();
  }
  display_initialized = false;
}

void Display::render_status_screen(const std::string &status_text,
                                   std::uint16_t bg, std::uint16_t fg) {
  if (!display_initialized) {
    return;
  }

  Paint_SelectImage(reinterpret_cast<UWORD *>(frame_buffer.data()));
  Paint_Clear(static_cast<UWORD>(bg));

  sFONT *font = &Font20;
  const int text_width =
      static_cast<int>(status_text.size()) * static_cast<int>(font->Width);
  const int text_height = static_cast<int>(font->Height);
  const int x = std::max(0, (width - text_width) / 2);
  const int y = std::max(0, (height - text_height) / 2);

  Paint_DrawString_EN(static_cast<UWORD>(x), static_cast<UWORD>(y),
                      status_text.c_str(), font, static_cast<UWORD>(fg),
                      static_cast<UWORD>(bg));
  flush();
}

void Display::show_status(const std::string &text) {
  std::cout << text << std::endl;
  render_status_screen(text, BLUE, WHITE);
}

std::string Display::show_tts_status() {
  render_status_screen("TTS READY", BLUE, WHITE);
  return "TTS status displayed";
}

std::string Display::show_ocr_status() {
  render_status_screen("OCR READY", GREEN, BLACK);
  return "OCR status displayed";
}

std::string Display::show_preview_feed() {
  render_status_screen("PREVIEW", MAGENTA, WHITE);
  return "Preview feed displayed";
}

void Display::flush() {
  if (!display_initialized) {
    return;
  }
  LCD_2IN_Display(reinterpret_cast<UBYTE *>(frame_buffer.data()));
}

} // namespace display
