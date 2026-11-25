#include "ui/screens/PageChapterText.h"

#include <algorithm>
#include <array>

#include "text/UnicodeText.h"
#include "text/UnicodeUppercase.generated.h"
#include "text/Utf8Text.h"

namespace screens::PageReader {
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

    std::vector<ChapterText::Word> ChapterText::headingWords(std::string_view title, bool& bidi) {
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

    void ChapterText::prepare(const ReadingSession& session) {
        headings_.clear();
        range_ = {};
        const auto bodyCount = ReadingLoop::wordCount(session);
        size_t inserted = 0;
        for (const auto& chapter: session.metadata.chapters) {
            if (chapter.wordIndex >= bodyCount || (!headings_.empty() && chapter.wordIndex <= headings_.back().bookWord))
                continue;
            bool bidi = false;
            auto words = headingWords(chapter.title, bidi);
            if (!words.empty()) {
                const size_t pageStart = chapter.wordIndex + inserted;
                inserted += words.size();
                headings_.push_back({chapter.wordIndex, pageStart, std::move(words), bidi});
            }
        }
        wordCount_ = bodyCount + inserted;
    }

    const ChapterText::Range& ChapterText::findRange(size_t position) const {
        const auto next = std::ranges::upper_bound(headings_, position, {}, &Heading::pageStart);
        const size_t end = next == headings_.end() ? wordCount_ : next->pageStart;
        if (next == headings_.begin()) {
            range_ = {0, end, 0, nullptr};
        } else {
            const auto& previous = *std::prev(next);
            if (position < previous.end())
                range_ = {previous.pageStart, previous.end(), previous.pageStart - previous.bookWord, &previous};
            else
                range_ = {previous.end(), end, previous.end() - previous.bookWord, nullptr};
        }
        return range_;
    }

    size_t ChapterText::pageWord(size_t position) const {
        const auto next = std::ranges::upper_bound(headings_, position, {}, &Heading::bookWord);
        if (next == headings_.begin())
            return position;
        const auto& previous = *std::prev(next);
        return position + previous.end() - previous.bookWord;
    }

    std::optional<uint32_t> ChapterText::titleWord(size_t position) const {
        const auto& range = rangeAt(position);
        if (range.heading)
            return static_cast<uint32_t>(position - range.first);
        return {};
    }

    std::optional<size_t> ChapterText::headingWord(size_t chapter, uint32_t word) const {
        const auto found = std::ranges::lower_bound(headings_, chapter, {}, &Heading::bookWord);
        if (found != headings_.end() && found->bookWord == chapter && word < found->words.size())
            return found->pageStart + word;
        return {};
    }

    std::pair<size_t, size_t> ChapterText::paragraphBounds(const ReadingSession& session, size_t position) const {
        const auto& range = rangeAt(position);
        if (range.heading)
            return {range.first, range.end};
        const auto [start, end] = ReadingLoop::paragraphBoundsAt(session, position - range.inserted);
        return {std::max(start + range.inserted, range.first), std::min(end + range.inserted, range.end)};
    }

    ReadingLoop::TextParagraph ChapterText::paragraphAt(const ReadingSession& session, size_t position) const {
        const auto [start, end] = paragraphBounds(session, position);
        const auto& range = rangeAt(position);
        if (!range.heading) {
            const size_t inserted = range.inserted;
            const auto [bodyStart, bodyEnd] = ReadingLoop::paragraphBoundsAt(session, position - inserted);
            if (start == bodyStart + inserted && end == bodyEnd + inserted) {
                auto paragraph = ReadingLoop::paragraphAt(session, position - inserted);
                paragraph.firstWord = start;
                paragraph.lastWord = end;
                return paragraph;
            }
        }
        ReadingLoop::TextParagraph paragraph{.firstWord = start, .lastWord = end};
        bool previousCjk = false;
        for (size_t index = start; index < end; ++index) {
            const auto word = wordAt(session, index);
            const bool cjk = UnicodeText::isCjkText(word);
            if (index != start && !(previousCjk && cjk))
                paragraph.text += ' ';
            paragraph.wordOffsets.push_back(paragraph.text.size());
            paragraph.text += word;
            previousCjk = cjk;
        }
        paragraph.wordOffsets.push_back(paragraph.text.size());
        return paragraph;
    }
}
