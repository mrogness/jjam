# TTS module

Turns OCR text into one MP3 per sentence using [Piper](https://github.com/OHF-Voice/piper1-gpl) and LAME.

## Setup (Raspberry Pi)

```bash
sudo apt install libmp3lame-dev
pipx install piper-tts            # or: pip install piper-tts in a virtual environment
mkdir -p ~/piper-voices && cd ~/piper-voices
python3 -m piper.download_voices en_US-lessac-medium
```

## Build

From the top of the repo:

```bash
cmake -S . -B build -DPIPER_MODEL=$HOME/piper-voices/en_US-lessac-medium.onnx
cmake --build build
```

If `piper` isn't on your PATH, add `-DPIPER_EXECUTABLE=/full/path/to/piper`.

## Run

```bash
./build/tts/tts_reader page.txt output/          # makes output/sentence_001.mp3 ...
./build/tts/tts_reader --split-only page.txt     # just print the sentences
```

`output/index.txt` lists each MP3 with the sentence it contains, in reading order.
Running again on a new page removes the old `sentence_*.mp3` files first.

## Using it from other code

Link to the `tts` library in CMake (`target_link_libraries(ocr_app PRIVATE tts)`), then:

```cpp
#include "tts/tts_pipeline.h"

tts::PiperTTS piper("piper", "/home/pi/piper-voices/en_US-lessac-medium.onnx");
tts::textFileToMp3s("page.txt", "output", piper,
    [](std::size_t i, std::size_t total, const tts::SentenceAudio& a) {
        // a.mp3Path is ready to play
    });
```
