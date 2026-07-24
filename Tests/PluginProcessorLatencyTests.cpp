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

    // Diagnostics only surfaced by Catch2 if a CHECK below actually fails - if
    // totalBlocksReceived/lastWrittenBlockPeak are 0 too, pushAudioBlock() itself never wrote
    // anything, which is a very different problem from a too-tight timing margin.
    INFO("totalBlocksReceived=" << processor.audioCapture.totalBlocksReceived.load());
    INFO("lastWrittenBlockPeak=" << processor.audioCapture.lastWrittenBlockPeak.load());

    // 300ms sleep / 100ms floor: a wide margin against scheduler jitter on shared/virtualized CI
    // runners - this is a regression guard against getCurrentLatencyMs() being stuck at 0 (a
    // previously fixed bug), not a precise timing measurement, so generous slack costs nothing.
    juce::Thread::sleep(300);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    const double latencyMs = processor.getCurrentLatencyMs();
    INFO("latencyMs=" << latencyMs);
    CHECK(std::isfinite(latencyMs));
    CHECK(latencyMs >= 100.0);
}

TEST_CASE("PluginAudioProcessor::processBlock settles latency back to 0 once fully drained and idle", "[PluginProcessorLatency]")
{
    PluginAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    std::vector<float> captured(256, 0.1f);
    const float* channelData[2] { captured.data(), captured.data() };
    processor.audioCapture.pushAudioBlock(channelData, 2, 256, 48000.0);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi); // drains the pushed block

    // Regression guard: further processBlock calls with nothing new pushed (e.g. after capture
    // is stopped, since processBlock keeps draining the now-empty buffer regardless) must settle
    // at 0, not keep growing against an increasingly stale production timestamp.
    juce::Thread::sleep(50);
    processor.processBlock(buffer, midi);
    CHECK(processor.getCurrentLatencyMs() == 0.0);
}

TEST_CASE("PluginAudioProcessor::prepareToPlay does not compound latency across repeated calls (device switches)", "[PluginProcessorLatency]")
{
    PluginAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    std::vector<float> captured(256, 0.1f);
    const float* channelData[2] { captured.data(), captured.data() };
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    // Regression guard for the original bug: switching the standalone output device re-invokes
    // prepareToPlay on an already-live processor. That used to leave leftover FIFO backlog in
    // place across every switch, so latency kept climbing a little more each time instead of
    // settling back down once drained.
    for (int i = 0; i < 5; ++i)
    {
        processor.audioCapture.pushAudioBlock(channelData, 2, 256, 48000.0);
        processor.processBlock(buffer, midi);

        processor.prepareToPlay(48000.0, 512); // simulated device switch
        processor.audioCapture.pushAudioBlock(channelData, 2, 256, 48000.0);

        processor.processBlock(buffer, midi); // first processBlock after the switch flushes that backlog

        CHECK(processor.getCurrentLatencyMs() == 0.0);
    }
}
