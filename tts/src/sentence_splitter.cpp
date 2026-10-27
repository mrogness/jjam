#include "tts/sentence_splitter.h"

#include <cctype>

namespace tts {

namespace {

std::string trim(const std::string& s) {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(begin, end - begin);
}

}  // namespace

std::vector<std::string> splitIntoSentences(const std::string& text) {
    std::vector<std::string> sentences;
    std::string current;

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];

        // Line breaks inside a sentence become spaces.
        current += (c == '\n' || c == '\r') ? ' ' : c;

        // A sentence ends at . ! or ? followed by whitespace or the end of the text.
        const bool isEndMark = (c == '.' || c == '!' || c == '?');
        const bool nextIsSpaceOrEnd =
            (i + 1 == text.size()) || std::isspace(static_cast<unsigned char>(text[i + 1]));

        if (isEndMark && nextIsSpaceOrEnd) {
            const std::string sentence = trim(current);
            if (!sentence.empty()) sentences.push_back(sentence);
            current.clear();
        }
    }

    // Any text left over (no final punctuation) is the last sentence.
    const std::string last = trim(current);
    if (!last.empty()) sentences.push_back(last);

    return sentences;
}

}  // namespace tts
