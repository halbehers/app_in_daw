#include <catch2/catch_test_macros.hpp>

#include "ProcessCategory.h"

namespace
{
    audiocapture::ProcessInfo makeProcess(const std::string& name, const std::string& path)
    {
        audiocapture::ProcessInfo info;
        info.name = name;
        info.executablePath = path;
        return info;
    }

    // A fresh, non-existent file per test so runs never see stale state from a previous run and
    // never touch the real user overrides file (~/Library/Nierika/<AppName>/process_categories.json).
    juce::File makeTempOverridesFile()
    {
        auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("ProcessCategoryConfigTests_" + juce::Uuid().toString() + ".json");
        file.deleteFile();
        return file;
    }

    const juce::String minimalBundledJson = R"({
        "version": 1,
        "categories": [
            { "id": "creative_and_daw", "names": ["Ableton Live"], "paths": ["Ableton Live.app"] }
        ]
    })";
}

TEST_CASE("ProcessCategoryMatcher recognizes known apps by name", "[ProcessCategoryMatcher]")
{
    CHECK(ProcessCategoryMatcher::categorize(makeProcess("Spotify", "")) == ProcessCategory::MusicAndMedia);
    CHECK(ProcessCategoryMatcher::categorize(makeProcess("Firefox", "")) == ProcessCategory::Browsers);
    CHECK(ProcessCategoryMatcher::categorize(makeProcess("Slack", "")) == ProcessCategory::CommunicationAndMeetings);
    CHECK(ProcessCategoryMatcher::categorize(makeProcess("Ableton Live", "")) == ProcessCategory::CreativeAndDAW);
}

TEST_CASE("ProcessCategoryMatcher falls back to executablePath when the process name is generic", "[ProcessCategoryMatcher]")
{
    const auto process = makeProcess("Helper (Renderer)", "/Applications/zoom.us.app/Contents/Frameworks/Helper.app/Contents/MacOS/Helper");
    CHECK(ProcessCategoryMatcher::categorize(process) == ProcessCategory::CommunicationAndMeetings);
}

TEST_CASE("ProcessCategoryMatcher returns nullopt for unrecognized processes", "[ProcessCategoryMatcher]")
{
    const auto process = makeProcess("com.apple.WindowServer", "/System/Library/PrivateFrameworks/SkyLight.framework/Resources/WindowServer");
    CHECK(ProcessCategoryMatcher::categorize(process) == std::nullopt);
}

TEST_CASE("ProcessCategoryMatcher::getDisplayName returns non-empty text for every category", "[ProcessCategoryMatcher]")
{
    CHECK(ProcessCategoryMatcher::getDisplayName(ProcessCategory::All).isNotEmpty());
    CHECK(ProcessCategoryMatcher::getDisplayName(ProcessCategory::MusicAndMedia).isNotEmpty());
    CHECK(ProcessCategoryMatcher::getDisplayName(ProcessCategory::Browsers).isNotEmpty());
    CHECK(ProcessCategoryMatcher::getDisplayName(ProcessCategory::CommunicationAndMeetings).isNotEmpty());
    CHECK(ProcessCategoryMatcher::getDisplayName(ProcessCategory::CreativeAndDAW).isNotEmpty());
}

TEST_CASE("ProcessCategoryConfig::build merges user overrides additively on top of bundled defaults", "[ProcessCategoryConfig]")
{
    const auto file = makeTempOverridesFile();
    file.replaceWithText(R"({
        "categories": [
            { "id": "creative_and_daw", "names": ["MyCustomDAW"], "paths": [] }
        ]
    })");

    const auto table = ProcessCategoryConfig::build(minimalBundledJson, file);

    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("Ableton Live", "")) == ProcessCategory::CreativeAndDAW);
    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("MyCustomDAW", "")) == ProcessCategory::CreativeAndDAW);

    file.deleteFile();
}

TEST_CASE("ProcessCategoryConfig::build falls back cleanly when the user overrides file is missing", "[ProcessCategoryConfig]")
{
    const auto file = makeTempOverridesFile(); // never created

    const auto table = ProcessCategoryConfig::build(minimalBundledJson, file);

    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("Ableton Live", "")) == ProcessCategory::CreativeAndDAW);
    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("MyCustomDAW", "")) == std::nullopt);
}

TEST_CASE("ProcessCategoryConfig::build ignores a malformed user overrides file without crashing", "[ProcessCategoryConfig]")
{
    const auto file = makeTempOverridesFile();
    file.replaceWithText("{ this is not valid json");

    const auto table = ProcessCategoryConfig::build(minimalBundledJson, file);

    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("Ableton Live", "")) == ProcessCategory::CreativeAndDAW);

    file.deleteFile();
}

TEST_CASE("ProcessCategoryConfig::build skips an unknown category id without dropping the rest of the file", "[ProcessCategoryConfig]")
{
    const auto file = makeTempOverridesFile();
    file.replaceWithText(R"({
        "categories": [
            { "id": "not_a_real_category", "names": ["Foo"], "paths": [] },
            { "id": "browsers", "names": ["MyBrowser"], "paths": [] }
        ]
    })");

    const auto table = ProcessCategoryConfig::build(minimalBundledJson, file);

    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("Foo", "")) == std::nullopt);
    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("MyBrowser", "")) == ProcessCategory::Browsers);

    file.deleteFile();
}

TEST_CASE("ProcessCategoryConfig::build falls back cleanly when the bundled JSON itself is malformed", "[ProcessCategoryConfig]")
{
    const auto table = ProcessCategoryConfig::build("not json at all", juce::File());
    CHECK(ProcessCategoryConfig::categorizeUsing(table, makeProcess("Ableton Live", "")) == std::nullopt);
}
