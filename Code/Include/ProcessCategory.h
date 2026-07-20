#pragma once

#include <audio_capture_dsp/audio_capture_dsp.h>

#include <juce_core/juce_core.h>

#include <optional>
#include <vector>

enum class ProcessCategory
{
    All,
    MusicAndMedia,
    Browsers,
    CommunicationAndMeetings,
    CreativeAndDAW,
};

struct ProcessCategoryMatcher
{
    static std::optional<ProcessCategory> categorize(const audiocapture::ProcessInfo& process);

    static juce::String getDisplayName(ProcessCategory category);
};

// Backs ProcessCategoryMatcher::categorize() with a config-driven table instead of a hardcoded
// one: a bundled default JSON (BinaryData) always applies, additively merged with an optional
// user overrides file so end users can add their own app names/paths without recompiling.
namespace ProcessCategoryConfig
{
    struct CategoryEntry
    {
        ProcessCategory category;
        std::vector<juce::String> nameSubstrings;
        std::vector<juce::String> pathSubstrings;
    };

    // Canonical mapping between a category and the JSON config's string id. Shared by build()'s
    // parsing and by AppSettings' category-pin persistence so there's exactly one id table.
    // categoryToConfigId must never be called with ProcessCategory::All - a UI-only pseudo-category
    // with no config id, never produced by build()/categorizeUsing().
    juce::String categoryToConfigId(ProcessCategory category);
    std::optional<ProcessCategory> configIdToCategory(const juce::String& id);

    // Parses bundledJsonText (always applied), then additively merges userOverridesFile on top
    // if it exists (bundled + user substrings concatenated per category, never replaced).
    // Malformed JSON, an unreadable file, an unknown category id, or a missing "categories"
    // array are all logged and skipped rather than thrown - a missing userOverridesFile is the
    // normal, silent case. Pure/read-only: never writes to disk.
    std::vector<CategoryEntry> build(const juce::String& bundledJsonText, const juce::File& userOverridesFile);

    // Same substring-matching logic as ProcessCategoryMatcher::categorize(), against an explicit
    // table rather than the cached singleton - lets tests exercise build()'s output directly.
    std::optional<ProcessCategory> categorizeUsing(const std::vector<CategoryEntry>& table,
                                                    const audiocapture::ProcessInfo& process);

    // Best-effort, idempotent: writes a valid, all-empty-arrays JSON template to the user
    // overrides path if it doesn't exist yet, so the user has a real file with the right
    // schema/ids to edit. Deliberately not called from categorize()/build() - see
    // ProcessCategory.cpp - so headless unit tests never write into the real user directory.
    void ensureUserOverridesTemplateExists();
}
