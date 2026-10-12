// Minimal callback structure for button press handling.
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "../display/display.h"

class Button {
  /**
   * Minimal representation of a hardware button.
   * Supports registering "callback" functions that are triggered on press.
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
  display::Display main_display("Main Display", true);
  Button button;

  // Register callback functions to run when button is pressed.
  button.add_on_press_callback([&main_display]() {
    std::cout << "Button pressed: " << main_display.show_preview_feed() << "\n";
  });

  // For demonstration purposes, but this will have to become more
  // complicated as we add more functionality
  std::cout << "Type 'p' to simulate button press, 'q' to quit.\n";
  std::string input;
  while (std::getline(std::cin, input)) {
    if (input == "q") {
      break;
    }

    if (input == "p") {
      button.press();
    } else {
      std::cout << "Unknown input. Use 'p' or 'q'.\n";
    }
  }

  return 0;
}