#include <catch2/catch_test_macros.hpp>

#include "PluginProcessor.h"

#include <cmath>
#include <vector>

TEST_CASE("PluginAudioProcessor exposes 0 latency before any audio has been captured", "[PluginProcessorLatency]")
{
    PluginAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    CHECK(processor.getCurrentLatencyMs() == 0.0);
}

TEST_CASE("PluginAudioProcessor::processBlock updates getCurrentLatencyMs from real capture", "[PluginProcessorLatency]")
{
    PluginAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    std::vector<float> captured(256, 0.1f);
    const float* channelData[2] { captured.data(), captured.data() };
    processor.audioCapture.pushAudioBlock(channelData, 2, 256, 48000.0);

    juce::Thread::sleep(50);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    const double latencyMs = processor.getCurrentLatencyMs();
    CHECK(std::isfinite(latencyMs));
    CHECK(latencyMs >= 30.0); // margin under the 50ms sleep to tolerate scheduler slop
}
