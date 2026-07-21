#pragma once

#include <nierika_dsp/nierika_dsp.h>
#include <audio_capture_dsp/audio_capture_dsp.h>

#include "PluginProcessor.h"
#include "ProcessCategory.h"
#include "Component/PerformanceMonitor.h"
#include "Component/CaptureStatus.h"
#include "Component/SettingsWindow.h"
#include "Component/ProcessTable.h"

class AppLayout final : public nlayout::AppLayout,
                         public nelement::SVGButton::OnClickListener,
                         public nelement::TextInput::OnValueChangedListener,
                         public nelement::ComboBox::OnValueChangedListener,
                         public nelement::TwoWaySwitch::OnValueChangedListener,
                         public component::ProcessTable::OnProcessCaptureListener,
                         public component::ProcessTable::OnProcessChosenListener
{
public:
    AppLayout(ndsp::ParameterManager& parameterManager, PluginAudioProcessor& audioProcessor);
    ~AppLayout() override;

    void bypassComponents(bool isBypassed) override;

    void resized() override;

    void onButtonClick(const std::string& componentID) override;
    void onValueChanged(const std::string& componentID, const std::string& newValue) override;
    void onSelectionChanged(const std::string& componentID, int selectedId) override;
    void onProcessCapture(const audiocapture::ProcessInfo& process) override;
    void onProcessChosen(const audiocapture::ProcessInfo& process) override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void populateCategoryFilter();

    PluginAudioProcessor& _audioProcessor;

    nelement::SVGButton _settings;
    nelement::TextInput _searchBar;
    nelement::ComboBox _categoryFilter;
    nelement::TwoWaySwitch _hideBackgroundSwitch;
    component::ProcessTable _processTable;

    component::PerformanceMonitor _performanceMonitor;
    component::CaptureStatus _captureStatus;
    nelement::SVGButton _captureButton;

    int _selectedProcessID = 0;

    nlayout::WindowsManager _windowsManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppLayout)
};
