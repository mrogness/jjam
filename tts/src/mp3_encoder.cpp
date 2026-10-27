#include "tts/mp3_encoder.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <lame/lame.h>

namespace fs = std::filesystem;

namespace tts {

namespace {

struct WavAudio {
    int channels = 0;
    int sampleRate = 0;
    std::vector<short> samples;  // 16-bit, interleaved if stereo
};

// WAV files store numbers little-endian.
std::uint16_t readU16(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}
std::uint32_t readU32(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

WavAudio readWav(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Could not open WAV file: " + path.string());
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),
                                           std::istreambuf_iterator<char>());

    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        throw std::runtime_error("Not a WAV file: " + path.string());
    }

    WavAudio wav;
    int format = 0;
    int bitsPerSample = 0;
    bool haveFormat = false;
    bool haveData = false;

    // A WAV file is a list of chunks: 4-byte id, 4-byte size, then the data.
    std::size_t pos = 12;
    while (pos + 8 <= bytes.size()) {
        const unsigned char* header = &bytes[pos];
        const std::size_t body = pos + 8;
        const std::size_t size = std::min<std::size_t>(readU32(header + 4), bytes.size() - body);

        if (std::memcmp(header, "fmt ", 4) == 0 && size >= 16) {
            format = readU16(&bytes[body]);
            wav.channels = readU16(&bytes[body + 2]);
            wav.sampleRate = static_cast<int>(readU32(&bytes[body + 4]));
            bitsPerSample = readU16(&bytes[body + 14]);
            haveFormat = true;
        } else if (std::memcmp(header, "data", 4) == 0) {
            wav.samples.resize(size / 2);
            for (std::size_t i = 0; i < wav.samples.size(); ++i) {
                wav.samples[i] = static_cast<short>(readU16(&bytes[body + 2 * i]));
            }
            haveData = true;
        }
        pos = body + size + (size & 1);  // chunks are padded to even sizes
    }

    if (!haveFormat || !haveData) {
        throw std::runtime_error("WAV file is missing its format or data: " + path.string());
    }
    if ((format != 1 && format != 0xFFFE) || bitsPerSample != 16) {
        throw std::runtime_error("Only 16-bit PCM WAV is supported: " + path.string());
    }
    if (wav.channels != 1 && wav.channels != 2) {
        throw std::runtime_error("Only mono or stereo WAV is supported: " + path.string());
    }
    if (wav.samples.empty()) {
        throw std::runtime_error("WAV file has no audio: " + path.string());
    }
    return wav;
}

// Makes sure lame_close() is always called, even if we throw.
struct LameHandle {
    lame_t gfp = lame_init();
    ~LameHandle() {
        if (gfp) lame_close(gfp);
    }
};

}  // namespace

void wavToMp3(const fs::path& wavPath, const fs::path& mp3Path, int bitrateKbps) {
    WavAudio wav = readWav(wavPath);

    LameHandle lame;
    if (!lame.gfp) throw std::runtime_error("Could not start the LAME MP3 encoder");

    lame_set_in_samplerate(lame.gfp, wav.sampleRate);
    lame_set_num_channels(lame.gfp, wav.channels);
    lame_set_mode(lame.gfp, wav.channels == 1 ? MONO : JOINT_STEREO);
    lame_set_brate(lame.gfp, bitrateKbps);  // constant bitrate
    lame_set_quality(lame.gfp, 5);          // 0 = best/slowest, 9 = worst/fastest
    lame_set_bWriteVbrTag(lame.gfp, 0);     // not needed for constant bitrate
    if (lame_init_params(lame.gfp) < 0) {
        throw std::runtime_error("Invalid MP3 encoder settings");
    }

    std::ofstream out(mp3Path, std::ios::binary);
    if (!out) throw std::runtime_error("Could not create " + mp3Path.string());

    // Encode in chunks. LAME's recommended worst-case buffer: 1.25 * n + 7200.
    const std::size_t chunkFrames = 8192;
    std::vector<unsigned char> mp3Buffer(chunkFrames * 5 / 4 + 7200);
    const std::size_t totalFrames = wav.samples.size() / wav.channels;

    for (std::size_t frame = 0; frame < totalFrames; frame += chunkFrames) {
        const int n = static_cast<int>(std::min(chunkFrames, totalFrames - frame));
        short* pcm = wav.samples.data() + frame * wav.channels;

        const int bytes =
            (wav.channels == 1)
                ? lame_encode_buffer(lame.gfp, pcm, pcm, n, mp3Buffer.data(),
                                     static_cast<int>(mp3Buffer.size()))
                : lame_encode_buffer_interleaved(lame.gfp, pcm, n, mp3Buffer.data(),
                                                 static_cast<int>(mp3Buffer.size()));
        if (bytes < 0) throw std::runtime_error("MP3 encoding failed");
        out.write(reinterpret_cast<const char*>(mp3Buffer.data()), bytes);
    }

    const int bytes = lame_encode_flush(lame.gfp, mp3Buffer.data(),
                                        static_cast<int>(mp3Buffer.size()));
    if (bytes < 0) throw std::runtime_error("MP3 encoding failed");
    out.write(reinterpret_cast<const char*>(mp3Buffer.data()), bytes);

    if (!out) throw std::runtime_error("Could not write " + mp3Path.string());
}

}  // namespace tts
