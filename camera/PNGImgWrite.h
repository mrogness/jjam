#ifndef PNGIMGWRITE_H
#define PNGIMGWRITE_H

#include <cstdint>
#include <vector>

/**
\class PNGImgWrite
\brief Write RGB data to a PNG file

Class to write a vector of RGB values (as std::uint8_t objects) to a PNG file
using the PNGwriter library
 */
class PNGImgWrite {
private:
    std::vector<std::uint8_t> rgbData;
    const unsigned char *data = nullptr;
    int width = 0;
    int height = 0;
    std::size_t stride = 0;
    const char *filename = nullptr;

public:
    /**
    Main constructor. Initialize new PNGImgWrite object  

    @param rgbData @c std::vector<std::uint8_t> Vector of RGB data that will be used to write to file
    @param width @c int Width of image
    @param height @c int Height of image
    @param stride @c std::size_t Stride that should be used when reading. This will be used to determine row seperation.
    @param filename @c const @c char @c * C-String object of what the file should be named
    */
    PNGImgWrite(std::vector<std::uint8_t> rgbData,
                int width,
                int height,
                std::size_t stride,
                const char *filename);

    PNGImgWrite() = default;

    /**
    Empty constructor. Use to allow object definition, but no initialization.
    For initialization, use the constructor with values.

    @see PNGImgWrite(std::vector<std::uint8_t> rgbData, int width, int height, std::size_t stride, const char *filename)
    */
    void writePNG(); /**< Create the PNG image based on information contained in object */
};

#endif