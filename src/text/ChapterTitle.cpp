#include "text/ChapterTitle.h"

#include <algorithm>
#include <array>
#include <utility>

#include "text/UnicodeText.h"
#include "text/UnicodeUppercase.generated.h"
#include "text/Utf8Text.h"

namespace ChapterTitle {
    namespace {
        void appendUppercase(std::string& word, uint32_t cp) {
            std::array<char, 4> bytes;
            const auto append = [&](uint32_t value) {
                const auto size = Utf8Text::encode(value, bytes);
                word.append(bytes.data(), size);
            };
            const auto& expansions = UnicodeUppercase::expansions;
            const auto expansion = std::ranges::lower_bound(expansions, cp, {}, &UnicodeUppercase::Expansion::source);
            if (expansion != std::ranges::end(expansions) && expansion->source == cp) {
                for (uint32_t value: expansion->result) {
                    if (value != 0)
                        append(value);
                }
                return;
            }

            const auto& ranges = UnicodeUppercase::ranges;
            const auto range = std::ranges::lower_bound(ranges, cp, {}, &UnicodeUppercase::Range::last);
            if (range != std::ranges::end(ranges) && cp >= range->first && (cp - range->first) % range->step == 0)
                cp = static_cast<uint32_t>(static_cast<int32_t>(cp) + range->delta);
            append(cp);
        }
    }

    std::vector<Word> words(std::string_view title, bool& bidi) {
        std::vector<Word> words;
        std::string display;
        const char* first = nullptr;
        const auto finishWord = [&](const char* end) {
            if (first) {
                words.push_back({std::exchange(display, {}), {first, static_cast<size_t>(end - first)}});
                first = nullptr;
            }
        };
        while (!title.empty()) {
            const char* start = title.data();
            uint32_t cp = 0;
            Utf8Text::next(title, cp);
            if (UnicodeText::isWhitespace(cp)) {
                finishWord(start);
            } else {
                if (!first)
                    first = start;
                bidi |= (UnicodeText::capabilityMask(cp) & UnicodeText::CapabilityBidi) != 0;
                appendUppercase(display, cp);
            }
        }
        finishWord(title.data());
        return words;
    }
}
