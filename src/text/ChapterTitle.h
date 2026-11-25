#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ChapterTitle {
    struct Word {
        std::string display;
        std::string_view original; // The chapter metadata owns the original text.
    };

    std::vector<Word> words(std::string_view title, bool& bidi);
}
