#include "ProcessFilter.h"

#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
 #include <windows.h>
#else
 #include <unistd.h>
#endif

namespace
{
    bool matchesSearchText(const audiocapture::ProcessInfo& process, const std::string& searchText)
    {
        if (searchText.empty())
            return true;

        const juce::String needle(searchText);
        return juce::String(process.name).containsIgnoreCase(needle)
            || juce::String(process.executablePath).containsIgnoreCase(needle);
    }

    bool matchesCategory(const audiocapture::ProcessInfo& process, ProcessCategory category)
    {
        if (category == ProcessCategory::All)
            return true;

        return ProcessCategoryMatcher::categorize(process) == category;
    }
}

int ProcessFilter::getCurrentProcessID()
{
#if JUCE_WINDOWS
    return (int) GetCurrentProcessId();
#else
    return (int) getpid();
#endif
}

std::vector<audiocapture::ProcessInfo> ProcessFilter::apply(
    const std::vector<audiocapture::ProcessInfo>& processes, const ProcessFilterOptions& options)
{
    std::vector<audiocapture::ProcessInfo> result;
    result.reserve(processes.size());

    const auto currentProcessID = getCurrentProcessID();

    for (const auto& process : processes)
    {
        if (process.processID == currentProcessID)
            continue;

        if (options.hideBackgroundProcesses && !process.isMainApplication)
            continue;

        if (!matchesCategory(process, options.category))
            continue;

        if (!matchesSearchText(process, options.searchText))
            continue;

        result.push_back(process);
    }

    return result;
}
