#pragma once

#include <audio_capture_dsp/audio_capture_dsp.h>

#include <string>
#include <vector>

#include "ProcessCategory.h"

struct ProcessFilterOptions
{
    std::string searchText;
    ProcessCategory category = ProcessCategory::All;
    bool hideBackgroundProcesses = false;
};

struct ProcessFilter
{
    static std::vector<audiocapture::ProcessInfo> apply(
        const std::vector<audiocapture::ProcessInfo>& processes,
        const ProcessFilterOptions& options);

    static int getCurrentProcessID();
};
