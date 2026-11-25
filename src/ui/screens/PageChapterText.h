#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "reader/ReadingLoop.h"
#include "text/ChapterTitle.h"

namespace screens::PageReader {
    // Page coordinates include title words. The underlying book and its word
    // indexes remain the source of truth for playback and the saved position.
    class ChapterText {
    public:
        void prepare(const ReadingSession& session);
        size_t count() const { return wordCount_; }
        bool empty() const { return headings_.empty(); }
        size_t bookWord(size_t position) const {
            const auto& range = rangeAt(position);
            return range.heading ? range.heading->bookWord : position - range.inserted;
        }
        size_t pageWord(size_t bookWord) const;
        std::optional<uint32_t> titleWord(size_t pageWord) const;
        std::optional<size_t> headingWord(size_t bookWord, uint32_t titleWord) const;
        std::string_view wordAt(const ReadingSession& session, size_t position) const {
            const auto& range = rangeAt(position);
            return range.heading ? std::string_view{range.heading->words[position - range.first].display}
                                 : ReadingLoop::wordAt(session, position - range.inserted);
        }
        std::string_view lookupWordAt(const ReadingSession& session, size_t position) const {
            const auto& range = rangeAt(position);
            return range.heading ? range.heading->words[position - range.first].original
                                 : ReadingLoop::wordAt(session, position - range.inserted);
        }
        std::pair<size_t, size_t> paragraphBounds(const ReadingSession& session, size_t pageWord) const;
        ReadingLoop::TextParagraph paragraphAt(const ReadingSession& session, size_t pageWord) const;
        bool heading(size_t pageWord) const { return rangeAt(pageWord).heading != nullptr; }
        bool headingBidi(size_t pageWord) const {
            const auto* heading = rangeAt(pageWord).heading;
            return heading && heading->bidi;
        }

    private:
        struct Heading {
            size_t bookWord;
            size_t pageStart;
            std::vector<ChapterTitle::Word> words;
            bool bidi;
            size_t end() const { return pageStart + words.size(); }
        };
        struct Range {
            size_t first = 0;
            size_t end = 0;
            size_t inserted = 0;
            const Heading* heading = nullptr;
        };
        const Range& rangeAt(size_t position) const {
            if (position >= range_.first && position < range_.end)
                return range_;
            return findRange(position);
        }
        const Range& findRange(size_t position) const;

        std::vector<Heading> headings_;
        size_t wordCount_ = 0;
        mutable Range range_;
    };
}
