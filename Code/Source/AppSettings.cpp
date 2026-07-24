#include "AppSettings.h"

namespace
{
    const char* SHOW_LATENCY_MONITOR_KEY = "showLatencyMonitor";
    const char* SHOW_STANDALONE_TITLE_KEY = "showStandaloneTitle";
    const char* THEME_MODE_KEY = "themeMode";
    const char* OUTPUT_TRIM_DB_KEY = "outputTrimDb";
    const char* LANGUAGE_KEY = "language";
    const char* LAST_CATEGORY_FILTER_KEY = "lastCategoryFilter";
    const char* HIDE_BACKGROUND_PROCESSES_KEY = "hideBackgroundProcesses";
    const char* CATEGORY_PINS_KEY = "categoryPins";

    const char* THEME_MODE_LIGHT = "light";
    const char* THEME_MODE_DARK = "dark";

    juce::String getAppName()
    {
        if (auto* app = juce::JUCEApplication::getInstance())
            return app->getApplicationName();

        const auto pluginName = juce::PluginDescription().name;
        if (pluginName.isNotEmpty())
            return pluginName;

        return "Default";
    }

    juce::File getDefaultSettingsFile()
    {
        return AppSettings::getAppSupportDirectory().getChildFile("settings.xml");
    }

    juce::PropertiesFile::Options makeOptions(juce::InterProcessLock& processLock)
    {
        juce::PropertiesFile::Options options;
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        options.processLock = &processLock;
        return options;
    }

    // Every entry: {"name": <string>, "path": <string>, "category": <config-id-string>}. Stored
    // compactly since it's opaque, app-managed state, not meant for hand-editing (unlike
    // process_categories.json's user-overrides file).
    juce::var loadCategoryPinsArray(const juce::PropertiesFile& properties)
    {
        juce::var parsed;
        if (juce::JSON::parse(properties.getValue(CATEGORY_PINS_KEY, "[]"), parsed).failed() || !parsed.isArray())
            return juce::var(juce::Array<juce::var>());
        return parsed;
    }

    // Mirrors PluginAudioProcessor::tryReacquireLastProcess()'s identity precedence exactly:
    // prefer an exact executablePath match when the query has a non-empty path, else fall back
    // to an exact name match.
    bool matchesPinIdentity(const juce::var& entry, const std::string& name, const std::string& executablePath)
    {
        if (!executablePath.empty())
            return entry["path"].toString() == juce::String(executablePath);
        return entry["name"].toString() == juce::String(name);
    }
}

AppSettings::AppSettings(const juce::File& settingsFile):
    _properties(settingsFile, makeOptions(_processLock))
{
}

bool AppSettings::getShowLatencyMonitor() const
{
    return _properties.getBoolValue(SHOW_LATENCY_MONITOR_KEY, true);
}

void AppSettings::setShowLatencyMonitor(bool show)
{
    _properties.setValue(SHOW_LATENCY_MONITOR_KEY, show);
    _properties.save();
}

bool AppSettings::getShowStandaloneTitle() const
{
    return _properties.getBoolValue(SHOW_STANDALONE_TITLE_KEY, true);
}

void AppSettings::setShowStandaloneTitle(bool show)
{
    _properties.setValue(SHOW_STANDALONE_TITLE_KEY, show);
    _properties.save();
}

nui::Theme::Mode AppSettings::getThemeMode() const
{
    return _properties.getValue(THEME_MODE_KEY, THEME_MODE_DARK) == THEME_MODE_LIGHT
        ? nui::Theme::Mode::LIGHT
        : nui::Theme::Mode::DARK;
}

void AppSettings::setThemeMode(nui::Theme::Mode mode)
{
    _properties.setValue(THEME_MODE_KEY, mode == nui::Theme::Mode::LIGHT ? THEME_MODE_LIGHT : THEME_MODE_DARK);
    _properties.save();
}

int AppSettings::getOutputTrimDb() const
{
    return _properties.getIntValue(OUTPUT_TRIM_DB_KEY, 0);
}

void AppSettings::setOutputTrimDb(int db)
{
    _properties.setValue(OUTPUT_TRIM_DB_KEY, db);
    _properties.save();
}

std::string AppSettings::getLanguage() const
{
    return _properties.getValue(LANGUAGE_KEY, "en").toStdString();
}

void AppSettings::setLanguage(const std::string& languageCode)
{
    _properties.setValue(LANGUAGE_KEY, juce::String(languageCode));
    _properties.save();
}

ProcessCategory AppSettings::getLastCategoryFilter() const
{
    return (ProcessCategory) _properties.getIntValue(LAST_CATEGORY_FILTER_KEY, (int) ProcessCategory::All);
}

void AppSettings::setLastCategoryFilter(ProcessCategory category)
{
    _properties.setValue(LAST_CATEGORY_FILTER_KEY, (int) category);
    _properties.save();
}

bool AppSettings::getHideBackgroundProcesses() const
{
    return _properties.getBoolValue(HIDE_BACKGROUND_PROCESSES_KEY, false);
}

void AppSettings::setHideBackgroundProcesses(bool hide)
{
    _properties.setValue(HIDE_BACKGROUND_PROCESSES_KEY, hide);
    _properties.save();
}

std::optional<ProcessCategory> AppSettings::getCategoryPin(const std::string& name, const std::string& executablePath) const
{
    const auto pins = loadCategoryPinsArray(_properties);
    if (const auto* array = pins.getArray())
        for (const auto& entry : *array)
            if (matchesPinIdentity(entry, name, executablePath))
                return ProcessCategoryConfig::configIdToCategory(entry["category"].toString());
    return std::nullopt;
}

void AppSettings::setCategoryPin(const std::string& name, const std::string& executablePath, ProcessCategory category)
{
    auto pins = loadCategoryPinsArray(_properties);
    auto* array = pins.getArray(); // never null: loadCategoryPinsArray() always returns an array var

    juce::DynamicObject::Ptr entry = new juce::DynamicObject();
    entry->setProperty("name", juce::String(name));
    entry->setProperty("path", juce::String(executablePath));
    entry->setProperty("category", ProcessCategoryConfig::categoryToConfigId(category));

    bool replaced = false;
    for (auto& existing : *array)
        if (matchesPinIdentity(existing, name, executablePath))
        {
            existing = juce::var(entry.get());
            replaced = true;
            break;
        }
    if (!replaced)
        array->add(juce::var(entry.get()));

    _properties.setValue(CATEGORY_PINS_KEY, juce::JSON::toString(pins, true));
    _properties.save();
}

void AppSettings::clearCategoryPin(const std::string& name, const std::string& executablePath)
{
    auto pins = loadCategoryPinsArray(_properties);
    auto* array = pins.getArray();

    for (int i = array->size(); --i >= 0;)
        if (matchesPinIdentity(array->getReference(i), name, executablePath))
            array->remove(i);

    _properties.setValue(CATEGORY_PINS_KEY, juce::JSON::toString(pins, true));
    _properties.save();
}

AppSettings& AppSettings::getInstance()
{
    static AppSettings& instance = *new AppSettings(getDefaultSettingsFile());
    return instance;
}

juce::File AppSettings::getAppSupportDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Nierika")
        .getChildFile(getAppName());
}
