#include "Parameters.h"

void Parameters::registerPluginParameters(PluginAudioProcessor* audioProcessor)
{
    audioProcessor->registerParameter
           (
               PLUGIN_ENABLED_ID,
               "Plugin Enabled",
               PLUGIN_ENABLED_DEFAULT,
               [audioProcessor](bool value) { /* TODO: Implement */ },
               "Bypass the whole plugin."
            );
}

void Parameters::registerSection(Section section, PluginAudioProcessor* audioProcessor)
{
    switch (section)
    {
        case PLUGIN:
            registerPluginParameters(audioProcessor);
            break;
    }
}

void Parameters::registerAllSections(PluginAudioProcessor* audioProcessor)
{
    registerSection(Section::PLUGIN, audioProcessor);
}
