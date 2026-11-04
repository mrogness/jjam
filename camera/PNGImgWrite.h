#ifndef PNGIMGWRITE_H
#define PNGIMGWRITE_H

#include <cstdint>
#include <vector>

class PNGImgWrite {
private:
    const unsigned char *data = nullptr;
    std::size_t bytesAvailable = 0;
    int width = 0;
    int height = 0;
    std::size_t stride = 0;
    const char *filename = nullptr;

public:
    PNGImgWrite(std::vector<std::uint8_t> &data,
                std::size_t bytesAvailable,
                int width,
                int height,
                std::size_t stride,
                const char *filename);

    PNGImgWrite() = default;

    void writePNG();
};

#endif