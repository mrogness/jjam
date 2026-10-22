#include <iostream>

#include "ocr.h"

int main() {

    // Define test files
    // Image will be scanned and output
    // will be written to text_out.txt 
    std::string imagePath = "ocr/images/test.png";

    std::string outputPath = "ocr/text/text_out.txt";

    // Run tesseract test
    // Stop with error code if tesseract fails
    if (!runOCR(imagePath, outputPath)) {
        std::cout << "Tesseract test failed."
                  << std::endl;

        return 1;
    }

    std::cout << "Tesseract test successful."
              << std::endl;

    return 0;
}