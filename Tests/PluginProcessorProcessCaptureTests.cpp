#include <catch2/catch_test_macros.hpp>

#include "PluginProcessor.h"

#include <vector>

TEST_CASE("PluginAudioProcessor::selectProcess updates capture state and stopCapturing clears it", "[PluginProcessorProcessCapture]")
{
    PluginAudioProcessor processor;
    CHECK(processor.isCapturing() == false);

    processor.selectProcess(4242, "Fixture App", "/fake/path/Fixture");
    CHECK(processor.isCapturing() == true);
    CHECK(processor.getLastProcessName() == "Fixture App");
    CHECK(processor.getLastProcessID() == 4242);

    processor.stopCapturing();
    CHECK(processor.isCapturing() == false);
}

TEST_CASE("PluginAudioProcessor::processBlock drains synthetic audio pushed directly into audioCapture", "[PluginProcessorProcessCapture]")
{
    // Mirrors PluginProcessorLatencyTests.cpp: bypasses the real SystemAudioTap (which needs OS
    // privacy consent and a live target, neither available in an automated test) by pushing
    // directly into the shared AudioCapture, exactly the way the real tap's callback would.
    PluginAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    std::vector<float> captured(256, 0.1f);
    const float* channelData[2] { captured.data(), captured.data() };
    processor.audioCapture.pushAudioBlock(channelData, 2, 256, 48000.0);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    CHECK(buffer.getMagnitude(0, 0, buffer.getNumSamples()) > 0.0f);
}
