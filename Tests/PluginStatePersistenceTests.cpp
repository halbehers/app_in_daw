#include <catch2/catch_test_macros.hpp>

#include "PluginProcessor.h"

TEST_CASE("PluginAudioProcessor persists and restores the last selected process", "[PluginStatePersistence]")
{
    PluginAudioProcessor sourceProcessor;
    sourceProcessor.selectProcess(4242, "Fixture App", "/Applications/Fixture.app/Contents/MacOS/Fixture");

    juce::MemoryBlock stateData;
    sourceProcessor.getStateInformation(stateData);
    REQUIRE(stateData.getSize() > 0);

    // A fresh instance, exactly matching what happens when the editor is recreated / the host
    // reloads a project - this is the whole point of the feature under test.
    PluginAudioProcessor restoredProcessor;
    restoredProcessor.setStateInformation(stateData.getData(), (int) stateData.getSize());

    // The fixture path/PID don't correspond to a real running process, so re-acquisition should
    // no-op (not crash, not falsely report capturing) - the persisted identity is still exactly
    // what was saved.
    CHECK(restoredProcessor.getLastProcessName() == "Fixture App");
    CHECK(restoredProcessor.getLastProcessExecutablePath() == "/Applications/Fixture.app/Contents/MacOS/Fixture");
    CHECK(restoredProcessor.getLastProcessID() == 4242);
    CHECK(restoredProcessor.isCapturing() == false);
}

TEST_CASE("PluginAudioProcessor round-trips cleanly with no process ever selected", "[PluginStatePersistence]")
{
    PluginAudioProcessor sourceProcessor;

    juce::MemoryBlock stateData;
    sourceProcessor.getStateInformation(stateData);

    PluginAudioProcessor restoredProcessor;
    restoredProcessor.setStateInformation(stateData.getData(), (int) stateData.getSize());

    CHECK(restoredProcessor.getLastProcessName().empty());
    CHECK(restoredProcessor.getLastProcessExecutablePath().empty());
    CHECK(restoredProcessor.isCapturing() == false);
}

TEST_CASE("PluginAudioProcessor doesn't accumulate duplicate state children across repeated getStateInformation calls", "[PluginStatePersistence]")
{
    PluginAudioProcessor processor;
    processor.selectProcess(1, "First", "/first");

    juce::MemoryBlock state;
    processor.getStateInformation(state);
    const int childCountAfterFirst = processor.getState().state.getNumChildren();

    processor.selectProcess(2, "Second", "/second");
    processor.getStateInformation(state);
    const int childCountAfterSecond = processor.getState().state.getNumChildren();

    CHECK(childCountAfterFirst == childCountAfterSecond);

    // And the latest call's data is genuinely what gets restored, not a stale duplicate.
    PluginAudioProcessor restored;
    restored.setStateInformation(state.getData(), (int) state.getSize());
    CHECK(restored.getLastProcessName() == "Second");
}

TEST_CASE("PluginAudioProcessor never trusts a stale stored PID to re-resolve, only path or name", "[PluginStatePersistence]")
{
    // Simulates a previously-captured process having exited, with a *different* real process now
    // happening to reuse the same PID - re-acquisition must not treat that as a match; only
    // path/name identity counts, the raw PID is informational-only.
    PluginAudioProcessor processor;

    const auto realProcesses = audiocapture::ProcessList::getAllProcesses();
    if (realProcesses.empty())
        return; // ProcessList unsupported on this platform/CI runner - nothing to verify here

    const auto& realProcess = realProcesses.front();

    processor.selectProcess(realProcess.processID, "Definitely Not This Process", "/definitely/not/this/path");

    juce::MemoryBlock state;
    processor.getStateInformation(state);

    PluginAudioProcessor restored;
    restored.setStateInformation(state.getData(), (int) state.getSize());

    // Neither the stored name nor path matches the real process at that PID, so re-acquisition
    // must not silently latch onto it just because the PID happens to line up.
    CHECK(restored.isCapturing() == false);
}
