#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <nierika_dsp/nierika_dsp.h>

#include "ProcessCategory.h"

class AppSettings
{
public:
    explicit AppSettings(const juce::File& settingsFile);

    [[nodiscard]] bool getShowLatencyMonitor() const;
    void setShowLatencyMonitor(bool show);

    [[nodiscard]] nui::Theme::Mode getThemeMode() const;
    void setThemeMode(nui::Theme::Mode mode);

    [[nodiscard]] int getOutputTrimDb() const;
    void setOutputTrimDb(int db);

    [[nodiscard]] std::string getLanguage() const;
    void setLanguage(const std::string& languageCode);

    [[nodiscard]] ProcessCategory getLastCategoryFilter() const;
    void setLastCategoryFilter(ProcessCategory category);

    [[nodiscard]] bool getHideBackgroundProcesses() const;
    void setHideBackgroundProcesses(bool hide);

    static AppSettings& getInstance();

    // Writable per-app support directory (~/Library/Nierika/<AppName> on macOS) this app uses
    // for all local, writable state - shared by settings storage and other per-app writable
    // files such as the process-categories user overrides.
    [[nodiscard]] static juce::File getAppSupportDirectory();

private:
    juce::InterProcessLock _processLock { "appindaw-settings" };
    juce::PropertiesFile _properties;
};
