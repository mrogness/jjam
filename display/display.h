// High level interface for managing the display component of the hardware system.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace display {
class Display {
private:
  bool display_initialized;
  std::string display_name;
  int width;
  int height;
  std::vector<std::uint16_t> frame_buffer;

  bool initialize_spi_display();
  void shutdown_spi_display();
  void flush();
  void render_status_screen(const std::string &status_text, std::uint16_t bg,
                            std::uint16_t fg);

public:
  Display(std::string display_name, bool initialized);
  ~Display();

  Display(const Display &) = delete;
  Display &operator=(const Display &) = delete;

  void show_status(const std::string &text);
  std::string show_tts_status();
  std::string show_ocr_status();
  std::string show_preview_feed();
};
} // namespace display