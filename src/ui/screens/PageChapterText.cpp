#include "ui/screens/PageChapterText.h"

#include <algorithm>

#include "text/UnicodeText.h"

namespace screens::PageReader {
    void ChapterText::prepare(const ReadingSession& session) {
        headings_.clear();
        range_ = {};
        const auto bodyCount = ReadingLoop::wordCount(session);
        size_t inserted = 0;
        for (const auto& chapter: session.metadata.chapters) {
            if (chapter.wordIndex >= bodyCount || (!headings_.empty() && chapter.wordIndex <= headings_.back().bookWord))
                continue;
            bool bidi = false;
            auto words = ChapterTitle::words(chapter.title, bidi);
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
