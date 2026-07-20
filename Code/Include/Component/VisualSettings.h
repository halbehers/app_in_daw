#pragma once

#include <nierika_dsp/nierika_dsp.h>
#include <juce_graphics/juce_graphics.h>

namespace component
{

class VisualSettings : public nui::Component, public nelement::ToggleSwitch::OnValueChangedListener, public nelement::TwoWaySwitch::OnValueChangedListener
{
public:
    explicit VisualSettings(const std::string& identifier);
    ~VisualSettings() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void onToggleValueChanged(const std::string& componentID, bool isOn) override;
    void onSelectionChanged(const std::string& componentID, int selectedIndex) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    enum class ControlAlignment { Left, Right };
    static constexpr ControlAlignment CONTROL_ALIGNMENT = ControlAlignment::Left;
    [[nodiscard]] static float getControlX(const juce::Rectangle<float>& cell, float controlWidth);

    nelement::Text _title { "visual-settings-title", "", juce::translate("visual_settings_title").toStdString() };

    nelement::Text _latencyLabel { "settings-latency-label", "", juce::translate("visual_settings_latency_label").toStdString() };
    nelement::ToggleSwitch _latencyToggle { "settings-latency-toggle" };

    nelement::Text _themeLabel { "settings-theme-label", "", juce::translate("visual_settings_theme_label").toStdString() };
    nelement::TwoWaySwitch _themeSwitch { "settings-theme-toggle", juce::translate("visual_settings_dark_theme").toStdString(), juce::translate("visual_settings_light_theme").toStdString() };

    nlayout::GridLayout<nui::Component> _layout { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualSettings)
};

}
