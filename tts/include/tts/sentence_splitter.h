#pragma once

#include <string>
#include <vector>

namespace tts {

// Splits text into sentences. A sentence ends at '.', '!' or '?' followed by
// whitespace or the end of the text. Line breaks inside a sentence are
// turned into spaces.
std::vector<std::string> splitIntoSentences(const std::string& text);

}  // namespace tts
