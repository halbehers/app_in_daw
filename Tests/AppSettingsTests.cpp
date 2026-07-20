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
