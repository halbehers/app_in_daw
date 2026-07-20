#pragma once

#include "PluginProcessor.h"

struct Parameters
{
    // General.
    static constexpr char PLUGIN_ENABLED_ID[] = "plugin-enabled";
    static constexpr bool PLUGIN_ENABLED_DEFAULT = true;

    static constexpr char PLUGIN_STATE_ID[] = "AppInDAWState";
    static constexpr char PLUGIN_STATE_LAST_PROCESS_NAME_ID[] = "lastProcessName";
    static constexpr char PLUGIN_STATE_LAST_PROCESS_PATH_ID[] = "lastProcessExecutablePath";
    static constexpr char PLUGIN_STATE_LAST_PROCESS_PID_ID[] = "lastProcessID"; // informational only - never used to re-resolve, PIDs aren't stable across relaunches

    enum Section
    {
        PLUGIN,
    };

    static void registerPluginParameters(PluginAudioProcessor* audioProcessor);

    static void registerSection(Section section, PluginAudioProcessor* audioProcessor);
    static void registerAllSections(PluginAudioProcessor* audioProcessor);
};
