#include "PNGImgWrite.h"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <pngwriter.h>

PNGImgWrite::PNGImgWrite(std::vector<std::uint8_t> rgbData,
                int width,
                int height,
                std::size_t stride,
                const char *filename)
        : rgbData(rgbData),
        width(width),
        height(height),
        stride(stride),
        filename(filename) {
            data = this->rgbData.data();
        }

void PNGImgWrite::writePNG() {
    std::size_t bytesAvailable = this->rgbData.size();
    
    // Use PNGwrite to write RGB info to PNG file
    if (!data || width < 1 || height < 1) {
        throw std::runtime_error("Invalid image info");
    }

    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;

    if (stride < rowBytes) {
        throw std::runtime_error("Invalid image info: stride too small");
    }

    if (bytesAvailable < rowBytes ||
        static_cast<std::size_t>(height - 1) > (bytesAvailable - rowBytes) / stride) {
        throw std::runtime_error("Invalid image info: buffer too small");
    }

    // Write file
    std::cout << "init writer\n";
    pngwriter writer(width, height, 0, filename);

    std::cout << "plot loop\n";
    for (int y = 0; y < height; ++y) {
        const unsigned char *row =
            data + static_cast<std::size_t>(y) * stride;

        for (int x = 0; x < width; ++x) {
            const unsigned char *pixel =
                row + static_cast<std::size_t>(x) * 3;

            writer.plot(x + 1, height - y,
                        pixel[0] / 255.0,
                        pixel[1] / 255.0,
                        pixel[2] / 255.0);
        }
    }

    std::cout << "close writer\n";
    writer.close();
}