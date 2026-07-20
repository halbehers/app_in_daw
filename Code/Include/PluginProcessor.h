/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <nierika_dsp/nierika_dsp.h>
#include <audio_capture_dsp/audio_capture_dsp.h>

#include <atomic>
#include <memory>
#include <string>

#include "Capture/SelectedProcessCapture.h"

using BlockType = juce::AudioBuffer<float>;

//==============================================================================
/**
*/
class PluginAudioProcessor : public juce::AudioProcessor, public ndsp::ParameterManager, private juce::Timer
{
public:
    //==============================================================================
    PluginAudioProcessor();
    ~PluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (BlockType&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    acdsp::AudioCapture audioCapture;

    double getCurrentLatencyMs() const { return audioCapture.getCurrentLatencyMs(); }

    void setOutputTrimDb(int db) { _outputTrimGain.store(juce::Decibels::decibelsToGain((float) db)); }

    // Starts capturing the given process's audio, replacing whatever was being captured before.
    void selectProcess(int processID, const std::string& name, const std::string& executablePath);
    void stopCapturing();
    // Reflects whether a capture target is currently selected/active, not the SystemAudioTap's
    // literal internal state (ProcessAudioCapture doesn't expose that) - false right after
    // construction or stopCapturing(), true from selectProcess() onward (including the brief
    // retry window before the underlying tap actually starts).
    [[nodiscard]] bool isCapturing() const { return _isCapturing; }

    [[nodiscard]] const std::string& getLastProcessName() const { return _lastProcessName; }
    [[nodiscard]] const std::string& getLastProcessExecutablePath() const { return _lastProcessExecutablePath; }
    [[nodiscard]] int getLastProcessID() const { return _lastProcessID; } // informational only - never used to re-resolve

private:
    std::unique_ptr<capture::SelectedProcessCapture> _processCapture;

    std::string _lastProcessName;
    std::string _lastProcessExecutablePath;
    int _lastProcessID = 0;
    bool _isCapturing = false;

    std::atomic<float> _outputTrimGain { 1.0f };

    void tryReacquireLastProcess();

    void timerCallback() override;

    juce::AudioProcessorValueTreeState::ParameterLayout getParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginAudioProcessor)
};
