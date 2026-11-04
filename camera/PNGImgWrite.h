#ifndef PNGIMGWRITE_H
#define PNGIMGWRITE_H

#include <cstdint>
#include <vector>

class PNGImgWrite {
private:
    std::vector<std::uint8_t> rgbData;
    const unsigned char *data = nullptr;
    int width = 0;
    int height = 0;
    std::size_t stride = 0;
    const char *filename = nullptr;

public:
    PNGImgWrite(std::vector<std::uint8_t> rgbData,
                int width,
                int height,
                std::size_t stride,
                const char *filename);

    PNGImgWrite() = default;

    void writePNG();
};

#endif