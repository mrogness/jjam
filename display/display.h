// High level interface for managing the display component of the hardware system.
#pragma once
#include <string>
namespace display {
    class Display {
        private:
            // Initialize the display hardware
            bool display_initialized;
            std::string display_name;
        public:
            Display(std::string display_name, bool initialized);

            std::string show_tts_status();
            std::string show_ocr_status();
            std::string show_preview_feed();
    };
}