#pragma once

#include <nierika_dsp/nierika_dsp.h>

#include "../PluginProcessor.h"

namespace component
{

class CaptureStatus : public nui::Component
{
public:
    explicit CaptureStatus(PluginAudioProcessor& audioProcessor);
    ~CaptureStatus() override;

    void resized() override;

    void refresh();

    void setSelectedProcess(const std::string& processName);
    void resetSelectedProcess();

private:
    PluginAudioProcessor& _audioProcessor;
    std::optional<std::string> _selectedProcessName = std::nullopt;

    nelement::Text _label { "capture-status-label", "", juce::translate("capture_status_empty_state").toStdString() };
    nelement::Text _process { "capture-status-process", "", "" };

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaptureStatus)
};

}
