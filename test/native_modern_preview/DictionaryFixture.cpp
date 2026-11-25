// Synthetic storage only. DictionaryScreen.cpp is the production renderer.
#include "dictionary/Dictionary.h"
namespace dictionary {
std::string previewCase;
size_t previewLookupWord = SIZE_MAX;
std::string previewLookupText;
void Dictionary::open(ReadingSession& session, std::optional<size_t> word, screens::Screen returnScreen,
                      std::function<std::string_view(size_t)> pageWordAt, size_t pageWordCount) {
returnScreen_ = returnScreen;
pageWordAt_ = std::move(pageWordAt);
pageWordCount_ = pageWordCount;
previewLookupWord = word.value_or(SIZE_MAX);
query_=word ? std::string(wordAt(session, *word)) : "curious";previewLookupText=query_;lookupWord_=word.value_or(15);sense_=0;definitionOffsets_={0};lookup();
if(previewCase=="missing-pack"){definition_={};message="Copy english.oewn to /dictionaries on the SD card.";}
if(previewCase=="missing-word"){query_="unlistedword";definition_={};message="No offline entry. Try online lookup.";}
if(previewCase=="online-error"){definition_={};message="Could not connect to Wi-Fi. Offline lookup still works.";}
if(previewCase=="long"){definition_.senses[0].definition="Eager to investigate and learn, showing careful interest in the world around you. This deliberately long definition exercises wrapping, continuation and paging on the narrow display. It gives the reviewer a realistic way to check that a definition remains readable without shrinking the text or hiding the navigation controls.";definition_.senses.push_back({.pos="adjective",.definition="Unusual or surprising."});}
}
void Dictionary::lookup(){message="Open English Wordnet · CC BY 4.0";definition_={.word="curious",.source="Open English Wordnet",.license="CC BY 4.0",
.senses={{.pos="adjective",.definition="Eager to investigate and learn or learn more.",.example="A curious mind."}}};}
}
