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

    // Diagnostics only surfaced by Catch2 if the CHECK below fails - narrows down which stage of
    // the pipeline (push -> resample -> fifo write -> drain) came up empty. The lastSpeedRatio.../
    // lastFifoWrite... fields (audio_capture_dsp v0.3.1) cover the push/resample/fifo-write half.
    INFO("totalBlocksReceived=" << processor.audioCapture.totalBlocksReceived.load());
    INFO("lastWrittenBlockPeak=" << processor.audioCapture.lastWrittenBlockPeak.load());
    INFO("lastSpeedRatio=" << processor.audioCapture.lastSpeedRatio.load());
    INFO("lastPendingCountBeforeResample=" << processor.audioCapture.lastPendingCountBeforeResample.load());
    INFO("lastNumOutputSamplesRequested=" << processor.audioCapture.lastNumOutputSamplesRequested.load());
    INFO("lastInterpolatorUsedLeft=" << processor.audioCapture.lastInterpolatorUsedLeft.load());
    INFO("lastInterpolatorUsedRight=" << processor.audioCapture.lastInterpolatorUsedRight.load());
    INFO("lastFifoFreeSpaceBeforeWrite=" << processor.audioCapture.lastFifoFreeSpaceBeforeWrite.load());
    INFO("lastFifoWriteSize1=" << processor.audioCapture.lastFifoWriteSize1.load());
    INFO("lastFifoWriteSize2=" << processor.audioCapture.lastFifoWriteSize2.load());
    INFO("totalProcessCalls=" << processor.audioCapture.totalProcessCalls.load());
    INFO("lastRequestedBufferSize=" << processor.audioCapture.lastRequestedBufferSize.load());
    INFO("lastNumRead=" << processor.audioCapture.lastNumRead.load());
    INFO("totalSamplesRead=" << processor.audioCapture.totalSamplesRead.load());
    INFO("lastReadBlockPeak=" << processor.audioCapture.lastReadBlockPeak.load());
    CHECK(buffer.getMagnitude(0, 0, buffer.getNumSamples()) > 0.0f);
}
