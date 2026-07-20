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

    // Per-process manual category override, keyed by identity (see setCategoryPin). Checked by
    // ProcessCategoryMatcher::categorize() before the substring-based config table, so a pin
    // takes effect everywhere a category is used (filter, sort, display).
    [[nodiscard]] std::optional<ProcessCategory> getCategoryPin(const std::string& name, const std::string& executablePath) const;

    // Identity mirrors PluginAudioProcessor::tryReacquireLastProcess()'s precedence: lookups
    // prefer an exact executablePath match when non-empty, else fall back to name. Never call
    // with ProcessCategory::All - it's a UI-only pseudo-category, not a real one.
    void setCategoryPin(const std::string& name, const std::string& executablePath, ProcessCategory category);
    void clearCategoryPin(const std::string& name, const std::string& executablePath);

    static AppSettings& getInstance();

    // Writable per-app support directory (~/Library/Nierika/<AppName> on macOS) this app uses
    // for all local, writable state - shared by settings storage and other per-app writable
    // files such as the process-categories user overrides.
    [[nodiscard]] static juce::File getAppSupportDirectory();

private:
    juce::InterProcessLock _processLock { "appindaw-settings" };
    juce::PropertiesFile _properties;
};
