#pragma once

#include <nierika_dsp/nierika_dsp.h>
#include <juce_graphics/juce_graphics.h>

#include "PluginProcessor.h"

namespace component
{

class OutputSettings : public nui::Component, public nelement::ThreeWaySwitch::OnValueChangedListener
{
public:
    OutputSettings(const std::string& identifier, PluginAudioProcessor& audioProcessor);
    ~OutputSettings() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void onSelectionChanged(const std::string& componentID, int selectedIndex) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    enum class ControlAlignment { Left, Right };
    static constexpr ControlAlignment CONTROL_ALIGNMENT = ControlAlignment::Left;
    [[nodiscard]] static float getControlX(const juce::Rectangle<float>& cell, float controlWidth);

    static constexpr int TRIM_VALUES_DB[3] = { 0, -6, -12 };

    PluginAudioProcessor& _audioProcessor;

    nelement::Text _title { "output-settings-title", "", juce::translate("output_trim_title").toStdString() };
    nelement::ThreeWaySwitch _trimSwitch { "output-trim-switch", juce::translate("output_trim_0db").toStdString(), juce::translate("output_trim_neg6db").toStdString(), juce::translate("output_trim_neg12db").toStdString() };

    nlayout::GridLayout<nui::Component> _layout { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputSettings)
};

}
