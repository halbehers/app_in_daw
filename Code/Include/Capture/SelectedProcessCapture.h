#pragma once

#include <audio_capture_dsp/audio_capture_dsp.h>

#include <atomic>

namespace capture
{

// Captures whichever OS process ID was most recently set via setTargetProcessID() - e.g. from a
// ProcessTable row selection, or resolved from persisted name/path on plugin reload. Unlike a
// polling-based process locator, the PID here is already known up front - getProcessID() just
// hands it straight back, so the base class's retry loop resolves on its very first tick.
class SelectedProcessCapture : public audiocapture::ProcessAudioCapture
{
public:
    explicit SelectedProcessCapture(acdsp::AudioCapture& destinationCapture);

    // Does NOT itself start/stop capture - call startCapture() afterwards. Safe to call while
    // already capturing a different process: startCapture() stops any existing tap first.
    void setTargetProcessID(int processID);
    [[nodiscard]] int getTargetProcessID() const { return _targetProcessID.load(); }

protected:
    bool getProcessID(int& outProcessID) override;

private:
    std::atomic<int> _targetProcessID { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectedProcessCapture)
};

}
