#include <catch2/catch_test_macros.hpp>

#include "Parameters.h"
#include "PluginProcessor.h"

#include <set>
#include <string>
#include <vector>

TEST_CASE("PluginAudioProcessor constructs successfully, registering all parameter sections", "[Parameters]")
{
    // PluginAudioProcessor::getParameterLayout() calls Parameters::registerAllSections as part
    // of construction (see PluginProcessor.cpp) - this exercises that path without risking a
    // double-registration by calling registerAllSections a second time on an already-built processor.
    REQUIRE_NOTHROW(PluginAudioProcessor());
}

TEST_CASE("Parameters ID constants are all non-empty and mutually distinct", "[Parameters]")
{
    // A copy-paste ID collision here would silently corrupt either APVTS parameter registration
    // or the plugin-state ValueTree schema.
    const std::vector<std::string> ids {
        Parameters::PLUGIN_ENABLED_ID,
        Parameters::PLUGIN_STATE_ID,
        Parameters::PLUGIN_STATE_LAST_PROCESS_NAME_ID,
        Parameters::PLUGIN_STATE_LAST_PROCESS_PATH_ID,
        Parameters::PLUGIN_STATE_LAST_PROCESS_PID_ID,
    };

    for (const auto& id : ids)
        CHECK(! id.empty());

    const std::set<std::string> uniqueIds(ids.begin(), ids.end());
    CHECK(uniqueIds.size() == ids.size());
}
