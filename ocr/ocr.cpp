#include "ocr.h"

#include <fstream>
#include <iostream>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

bool runOCR(const std::string &imagePath,
            const std::string &outputPath) {

    // Create tesseract instance
    tesseract::TessBaseAPI api;

    // Initialize tesseract with English language
    if (api.Init("ocr/tessdata", "eng")) {
        std::cout << "Failed to initialize Tesseract."
                  << std::endl;

        return false;
    }

    // Load image
    Pix *image = pixRead(imagePath.c_str());

    if (!image) {
        std::cout << "Failed to load image: "
                  << imagePath << std::endl;

        api.End();

        return false;
    }

    // Run tesseract on image
    api.SetImage(image);

    char *text = api.GetUTF8Text();

    if (!text) {
        std::cout << "Tesseract failed."
                  << std::endl;

        pixDestroy(&image);
        api.End();

        return false;
    }

    // Create output file
    std::ofstream outputFile(outputPath);

    if (!outputFile.is_open()) {
        std::cout << "Failed to create output file."
                  << std::endl;

        delete[] text;

        pixDestroy(&image);
        api.End();

        return false;
    }

    // Save tesseract text to output file
    outputFile << text;

    std::cout << "Tesseract complete."
              << std::endl;

    std::cout << "Output saved to "
              << outputPath << std::endl;

    // Cleanup
    outputFile.close();

    delete[] text;

    pixDestroy(&image);

    api.End();

    return true;
}