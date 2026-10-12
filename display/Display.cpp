// Implementation of the display component interface.
#include "display.h"

namespace display {

Display::Display(std::string display_name, bool initialized)
    : display_initialized(initialized), display_name(display_name) {
    // Initialize the display hardware
}

std::string Display::show_tts_status() {
    // Show TTS status on the display
    return "TTS status displayed";
}

std::string Display::show_ocr_status() {
    // Show OCR status on the display
    return "OCR status displayed";
}

std::string Display::show_preview_feed() {
    // Show preview feed on the display
    return "Preview feed displayed";
}

} // namespace display