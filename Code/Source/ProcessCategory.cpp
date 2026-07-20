#include "ProcessCategory.h"
#include "AppSettings.h"
#include "BinaryData.h"

#include <nierika_dsp/nierika_dsp.h>

#include <map>

namespace
{
    constexpr const char* kLogSource = "ProcessCategory";

    std::optional<ProcessCategory> configIdToCategory(const juce::String& id)
    {
        if (id == "music_and_media")            return ProcessCategory::MusicAndMedia;
        if (id == "browsers")                   return ProcessCategory::Browsers;
        if (id == "communication_and_meetings") return ProcessCategory::CommunicationAndMeetings;
        if (id == "creative_and_daw")           return ProcessCategory::CreativeAndDAW;
        return std::nullopt;
    }

    // Fixed, deterministic output order, independent of JSON document order.
    const std::vector<ProcessCategory>& orderedCategories()
    {
        static const std::vector<ProcessCategory> categories = {
            ProcessCategory::MusicAndMedia, ProcessCategory::Browsers,
            ProcessCategory::CommunicationAndMeetings, ProcessCategory::CreativeAndDAW,
        };
        return categories;
    }

    std::vector<juce::String> readStringArray(const juce::var& parent, const char* key)
    {
        std::vector<juce::String> result;
        if (const auto* array = parent[key].getArray())
            for (const auto& value : *array)
                result.push_back(value.toString());
        return result;
    }

    void mergeDocument(const juce::String& jsonText, const juce::String& sourceLabel, bool isBundledSource,
                        std::map<ProcessCategory, ProcessCategoryConfig::CategoryEntry>& target)
    {
        auto logProblem = [isBundledSource](const juce::String& message)
        {
            if (isBundledSource)
                nutils::AppLogger::error(message.toStdString(), kLogSource);
            else
                nutils::AppLogger::warn(message.toStdString(), kLogSource);
        };

        juce::var root;
        if (const auto result = juce::JSON::parse(jsonText, root); result.failed())
        {
            logProblem("Failed to parse process category config '" + sourceLabel + "': " + result.getErrorMessage());
            return;
        }

        if (!root.isObject())
        {
            logProblem("Process category config '" + sourceLabel + "' is not a JSON object; ignoring.");
            return;
        }

        const auto* categoriesArray = root["categories"].getArray();
        if (categoriesArray == nullptr)
        {
            logProblem("Process category config '" + sourceLabel + "' has no 'categories' array; ignoring.");
            return;
        }

        for (const auto& entry : *categoriesArray)
        {
            if (!entry.isObject())
            {
                logProblem("Process category config '" + sourceLabel + "' contains a non-object category entry; skipping.");
                continue;
            }

            const auto id = entry["id"].toString();
            const auto category = configIdToCategory(id);
            if (!category.has_value())
            {
                logProblem("Process category config '" + sourceLabel + "' references unknown category id '" + id + "'; skipping.");
                continue;
            }

            auto& categoryEntry = target[*category];
            categoryEntry.category = *category;

            const auto names = readStringArray(entry, "names");
            const auto paths = readStringArray(entry, "paths");
            categoryEntry.nameSubstrings.insert(categoryEntry.nameSubstrings.end(), names.begin(), names.end());
            categoryEntry.pathSubstrings.insert(categoryEntry.pathSubstrings.end(), paths.begin(), paths.end());
        }
    }

    juce::File getUserOverridesFile()
    {
        return AppSettings::getAppSupportDirectory().getChildFile("process_categories.json");
    }

    const char* userOverridesTemplate()
    {
        return R"JSON({
    "_comment": "Add your own app names/paths to any category below. Matching is a case-insensitive substring check against a running process's name and executable path (e.g. \"Reason\" matches \"Reason 12.app\"). Entries here are merged additively on top of AppInDAW's bundled defaults - no need to repeat the bundled entries. Restart your DAW/the plugin instance to pick up changes.",
    "version": 1,
    "categories": [
        { "id": "music_and_media", "names": [], "paths": [] },
        { "id": "browsers", "names": [], "paths": [] },
        { "id": "communication_and_meetings", "names": [], "paths": [] },
        { "id": "creative_and_daw", "names": [], "paths": [] }
    ]
}
)JSON";
    }

    const std::vector<ProcessCategoryConfig::CategoryEntry>& getCategoryTable()
    {
        static const std::vector<ProcessCategoryConfig::CategoryEntry> table = ProcessCategoryConfig::build(
            juce::String::createStringFromData(BinaryData::process_categories_json, BinaryData::process_categories_jsonSize),
            getUserOverridesFile());
        return table;
    }
}

namespace ProcessCategoryConfig
{

std::vector<CategoryEntry> build(const juce::String& bundledJsonText, const juce::File& userOverridesFile)
{
    std::map<ProcessCategory, CategoryEntry> merged;

    mergeDocument(bundledJsonText, "bundled defaults", true, merged);

    if (userOverridesFile.existsAsFile())
        mergeDocument(userOverridesFile.loadFileAsString(), userOverridesFile.getFullPathName(), false, merged);
    // A missing user overrides file is the expected, common case - nothing to log.

    std::vector<CategoryEntry> table;
    table.reserve(orderedCategories().size());
    for (const auto category : orderedCategories())
    {
        if (const auto it = merged.find(category); it != merged.end())
            table.push_back(it->second);
        else
            table.push_back({ category, {}, {} });
    }
    return table;
}

std::optional<ProcessCategory> categorizeUsing(const std::vector<CategoryEntry>& table, const audiocapture::ProcessInfo& process)
{
    const juce::String name(process.name);
    const juce::String path(process.executablePath);

    for (const auto& entry : table)
    {
        for (const auto& substring : entry.nameSubstrings)
            if (name.containsIgnoreCase(substring))
                return entry.category;

        for (const auto& substring : entry.pathSubstrings)
            if (path.containsIgnoreCase(substring))
                return entry.category;
    }

    return std::nullopt;
}

void ensureUserOverridesTemplateExists()
{
    const auto file = getUserOverridesFile();
    if (file.existsAsFile())
        return;

    if (!file.getParentDirectory().createDirectory().wasOk())
    {
        nutils::AppLogger::warn("Could not create directory for process category overrides at '"
            + file.getParentDirectory().getFullPathName().toStdString() + "'.", kLogSource);
        return;
    }

    if (!file.replaceWithText(userOverridesTemplate()))
        nutils::AppLogger::warn("Could not write process category overrides template to '"
            + file.getFullPathName().toStdString() + "'.", kLogSource);
}

}

std::optional<ProcessCategory> ProcessCategoryMatcher::categorize(const audiocapture::ProcessInfo& process)
{
    return ProcessCategoryConfig::categorizeUsing(getCategoryTable(), process);
}

juce::String ProcessCategoryMatcher::getDisplayName(ProcessCategory category)
{
    switch (category)
    {
        case ProcessCategory::All:                       return juce::translate("category_all");
        case ProcessCategory::MusicAndMedia:              return juce::translate("category_music_and_media");
        case ProcessCategory::Browsers:                   return juce::translate("category_browsers");
        case ProcessCategory::CommunicationAndMeetings:   return juce::translate("category_communication_and_meetings");
        case ProcessCategory::CreativeAndDAW:             return juce::translate("category_creative_and_daw");
    }

    return {};
}
