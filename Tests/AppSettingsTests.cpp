#include <catch2/catch_test_macros.hpp>

#include "AppSettings.h"

namespace
{
    // A fresh, non-existent file per test so runs never see stale state from a previous run and
    // never touch the real user settings file (~/Library/Nierika/<AppName>/settings.xml).
    juce::File makeTempSettingsFile()
    {
        auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("AppSettingsTests_" + juce::Uuid().toString() + ".xml");
        file.deleteFile();
        return file;
    }
}

TEST_CASE("AppSettings returns sane defaults over a nonexistent file", "[AppSettings]")
{
    const auto file = makeTempSettingsFile();
    AppSettings settings(file);

    CHECK(settings.getShowLatencyMonitor() == true);
    CHECK(settings.getThemeMode() == nui::Theme::Mode::DARK);
    CHECK(settings.getOutputTrimDb() == 0);
    CHECK(settings.getLanguage() == "en");
    CHECK(settings.getLastCategoryFilter() == ProcessCategory::All);
    CHECK(settings.getHideBackgroundProcesses() == false);

    file.deleteFile();
}

TEST_CASE("AppSettings round-trips each typed accessor", "[AppSettings]")
{
    const auto file = makeTempSettingsFile();
    AppSettings settings(file);

    settings.setShowLatencyMonitor(false);
    CHECK(settings.getShowLatencyMonitor() == false);

    settings.setThemeMode(nui::Theme::Mode::LIGHT);
    CHECK(settings.getThemeMode() == nui::Theme::Mode::LIGHT);

    settings.setThemeMode(nui::Theme::Mode::DARK);
    CHECK(settings.getThemeMode() == nui::Theme::Mode::DARK);

    settings.setOutputTrimDb(-6);
    CHECK(settings.getOutputTrimDb() == -6);

    settings.setLanguage("fr");
    CHECK(settings.getLanguage() == "fr");

    settings.setLastCategoryFilter(ProcessCategory::MusicAndMedia);
    CHECK(settings.getLastCategoryFilter() == ProcessCategory::MusicAndMedia);

    settings.setHideBackgroundProcesses(true);
    CHECK(settings.getHideBackgroundProcesses() == true);

    file.deleteFile();
}

TEST_CASE("AppSettings persists across separate instances over the same file", "[AppSettings]")
{
    const auto file = makeTempSettingsFile();

    {
        AppSettings first(file);
        first.setShowLatencyMonitor(false);
        first.setThemeMode(nui::Theme::Mode::LIGHT);
        first.setLastCategoryFilter(ProcessCategory::Browsers);
        first.setHideBackgroundProcesses(true);
    }

    // A second instance, exactly matching what happens when a second plugin instance/DAW
    // session reads the same global settings file.
    AppSettings second(file);
    CHECK(second.getShowLatencyMonitor() == false);
    CHECK(second.getThemeMode() == nui::Theme::Mode::LIGHT);
    CHECK(second.getLastCategoryFilter() == ProcessCategory::Browsers);
    CHECK(second.getHideBackgroundProcesses() == true);

    file.deleteFile();
}

TEST_CASE("AppSettings category pins round-trip and support Auto (clear)", "[AppSettings]")
{
    const auto file = makeTempSettingsFile();
    AppSettings settings(file);

    CHECK(settings.getCategoryPin("Ableton Live", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live") == std::nullopt);

    settings.setCategoryPin("Ableton Live", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live", ProcessCategory::CreativeAndDAW);
    CHECK(settings.getCategoryPin("Ableton Live", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live") == ProcessCategory::CreativeAndDAW);

    // Re-pinning the same identity replaces rather than duplicates.
    settings.setCategoryPin("Ableton Live", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live", ProcessCategory::MusicAndMedia);
    CHECK(settings.getCategoryPin("Ableton Live", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live") == ProcessCategory::MusicAndMedia);

    settings.clearCategoryPin("Ableton Live", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live");
    CHECK(settings.getCategoryPin("Ableton Live", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live") == std::nullopt);

    file.deleteFile();
}

TEST_CASE("AppSettings category pin lookup prefers path over name, exactly like tryReacquireLastProcess", "[AppSettings]")
{
    const auto file = makeTempSettingsFile();
    AppSettings settings(file);

    settings.setCategoryPin("Renderer", "/Applications/Zoom.app/Contents/MacOS/Renderer", ProcessCategory::CommunicationAndMeetings);

    // Query has a non-empty path that differs from the stored one - path takes precedence over name, no match.
    CHECK(settings.getCategoryPin("Renderer", "/Applications/Other.app/Contents/MacOS/Renderer") == std::nullopt);

    // Query has an empty path - falls back to matching by name, which does match here.
    CHECK(settings.getCategoryPin("Renderer", "") == ProcessCategory::CommunicationAndMeetings);

    // Query has the exact same non-empty path - matches.
    CHECK(settings.getCategoryPin("Renderer", "/Applications/Zoom.app/Contents/MacOS/Renderer") == ProcessCategory::CommunicationAndMeetings);

    file.deleteFile();
}

TEST_CASE("AppSettings category pins persist across separate instances over the same file", "[AppSettings]")
{
    const auto file = makeTempSettingsFile();
    {
        AppSettings first(file);
        first.setCategoryPin("Spotify", "", ProcessCategory::MusicAndMedia);
    }
    AppSettings second(file);
    CHECK(second.getCategoryPin("Spotify", "") == ProcessCategory::MusicAndMedia);
    file.deleteFile();
}
