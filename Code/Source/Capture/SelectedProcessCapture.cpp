#include "Capture/SelectedProcessCapture.h"

namespace capture
{

SelectedProcessCapture::SelectedProcessCapture(acdsp::AudioCapture& destinationCapture)
    : ProcessAudioCapture(destinationCapture)
{
}

void SelectedProcessCapture::setTargetProcessID(int processID)
{
    _targetProcessID.store(processID);
}

bool SelectedProcessCapture::getProcessID(int& outProcessID)
{
    const auto pid = _targetProcessID.load();
    if (pid == 0)
        return false;

    outProcessID = pid;
    return true;
}

}
