// Defines the Hardware Loop for this project, responsible for managing the main control flow and interactions with the hardware components.
#include <iostream>
using namespace std;

#include "../display/display.h"
int main() {
    // Initialize hardware components here

    // Initialize display
    display::Display main_display = display::Display("Main Display", true);
    /*
    Main control loop
    Define Event Listeners for hardware events, and enacts appropriate responses
    Example: Print a message to indicate the loop is running
    */
    while (true) {
        cout << main_display.show_tts_status() << std::endl;
        cout << main_display.show_ocr_status() << std::endl;
        cout << main_display.show_preview_feed() << std::endl;
    }

    return 0;
}