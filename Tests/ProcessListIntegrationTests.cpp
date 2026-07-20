#include <catch2/catch_test_macros.hpp>

#include "ProcessFilter.h"

#include <algorithm>

TEST_CASE("ProcessList enumerates real OS processes when supported", "[ProcessListIntegration]")
{
    if (! audiocapture::ProcessList::isSupported())
        return; // Linux/other unsupported platform - nothing to verify here

    const auto processes = audiocapture::ProcessList::getAllProcesses();
    CHECK(! processes.empty());

    const auto currentProcessID = ProcessFilter::getCurrentProcessID();
    const auto selfFound = std::any_of(processes.begin(), processes.end(),
        [currentProcessID](const audiocapture::ProcessInfo& process) { return process.processID == currentProcessID; });

    // Proves ProcessFilter::apply's "exclude self" behavior (see ProcessFilterTests.cpp) is
    // filtering something real, not a no-op against an already-empty case.
    CHECK(selfFound);
}
