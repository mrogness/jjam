#ifndef OCR_H
#define OCR_H

#include <string>

// Read text from an image and save
// the OCR output to a text file
bool runOCR(const std::string &imagePath,
            const std::string &outputPath);

#endif