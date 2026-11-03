// Minimal callback structure for button press handling.
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "../display/display.h"
#include "../ocr/ocr.h"

// Integration points for the camera and TTS modules. 
// Mostly just included something as a placeholder
bool captureImage(const std::string &imagePath);
void processAndPlayAudio(const std::string &textPath);

class Button {
  /**
   * Minimal representation of a hardware button.
   * Supports registering "callback" functions that are triggered on press
   * Can be moved into a interface later perhaps?
   */
public:
  // equiv to Python: import callable as Callback
  using Callback = std::function<void()>;

  // allows registering a "callback" function
  void add_on_press_callback(Callback cb) {
    callbacks_.push_back(std::move(cb));
  }

  // simulates the button being pressed, which triggers all registered callback
  // functions
  void press() {
    for (const auto &cb : callbacks_) {
      cb();
    }
  }

private:
  std::vector<Callback> callbacks_;
};

int main() {
  // Hardware initialization and setup goes here
  display::Display main_display("Main Display", true);
  Button pButton;
  Button oButton;
  Button lButton;
  const std::string imagePath = "capture.jpg";
  const std::string textPath = "ocr/text/text_out.txt";
  bool textReady = false;

  //lambda function to process and play audio from the OCR text file -- should from tts probably?
  const auto playAudio = [&]() {
    if (!textReady) {
      std::cout << "Capture and recognize a page first.\n";
      return;
    }
    std::cout << main_display.show_tts_status() << "\n";
    processAndPlayAudio(textPath);
  };

  // Register callback functions to run when button is pressed.
  pButton.add_on_press_callback([&main_display]() {
    std::cout << "pButton pressed: " << main_display.show_preview_feed() << "\n";
  });

  // One press runs the stages in order. A failed stage stops this reading.
  oButton.add_on_press_callback([&]() {
    textReady = false;
    std::cout << "Capturing image...\n";
    if (!captureImage(imagePath)) {
      std::cout << "Camera capture failed.\n";
      return;
    }

    std::cout << main_display.show_ocr_status() << "\n";
    if (!runOCR(imagePath, textPath)) {
      std::cout << "OCR failed.\n";
      return;
    }

    textReady = true;
    playAudio();
  });
  lButton.add_on_press_callback(playAudio);
  std::cout << "Type 'p' for preview status, 'o' to capture/read/speak, "
               "'l' to process/play the latest text again, or 'q' to quit, "
               "then press Enter.\n";
  std::string input;
  while (std::getline(std::cin, input)) {
    if (input == "q") {
      break;
    }

    if (input == "p") {
      pButton.press();
    } else if (input =="o") {
      oButton.press();
    } else if (input == "l") {
      lButton.press();
    } else {
      std::cout << "Unknown input. Use 'p', 'o', 'l' or 'q'.\n";
    }
  }

  return 0;
}
