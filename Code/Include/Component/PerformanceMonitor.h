#pragma once

#include <nierika_dsp/nierika_dsp.h>

#include "../PluginProcessor.h"

namespace component
{

class PerformanceMonitor : public nui::Component, private juce::Timer
{
public:
    static constexpr bool ENABLED = true;
    static constexpr int LABEL_WIDTH = 150;
    static constexpr int VALUE_WIDTH = 80;

    explicit PerformanceMonitor(PluginAudioProcessor& audioProcessor);
    ~PerformanceMonitor() override;

    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    PluginAudioProcessor& _audioProcessor;

    nelement::Text _label { "latency-label", "", juce::translate("performance_monitor_latency_label").toStdString() };
    nelement::Value _latencyValue { "latency-value", "", juce::translate("performance_monitor_latency_unit").toStdString() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceMonitor)
};

}
