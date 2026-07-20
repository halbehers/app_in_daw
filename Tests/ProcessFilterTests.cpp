#include <catch2/catch_test_macros.hpp>

#include "ProcessFilter.h"

#include <vector>

namespace
{
    audiocapture::ProcessInfo makeProcess(int pid, const std::string& name, const std::string& path, bool isMainApplication)
    {
        audiocapture::ProcessInfo info;
        info.processID = pid;
        info.name = name;
        info.executablePath = path;
        info.isMainApplication = isMainApplication;
        return info;
    }

    std::vector<audiocapture::ProcessInfo> makeFixtureProcesses()
    {
        return {
            makeProcess(100, "Spotify", "/Applications/Spotify.app/Contents/MacOS/Spotify", true),
            makeProcess(101, "Spotify Helper (Renderer)", "/Applications/Spotify.app/Contents/Frameworks/Spotify Helper.app/Contents/MacOS/Spotify Helper", false),
            makeProcess(102, "Google Chrome", "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome", true),
            makeProcess(103, "com.apple.WindowServer", "/System/Library/PrivateFrameworks/SkyLight.framework/Resources/WindowServer", false),
        };
    }
}

TEST_CASE("ProcessFilter matches search text against name or path, case-insensitively", "[ProcessFilter]")
{
    const auto processes = makeFixtureProcesses();

    ProcessFilterOptions options;
    options.searchText = "spotify";
    const auto result = ProcessFilter::apply(processes, options);

    REQUIRE(result.size() == 2);
    CHECK(result[0].name == "Spotify");
    CHECK(result[1].name == "Spotify Helper (Renderer)");
}

TEST_CASE("ProcessFilter Music & Media category matches known apps by name", "[ProcessFilter]")
{
    const auto processes = makeFixtureProcesses();

    ProcessFilterOptions options;
    options.category = ProcessCategory::MusicAndMedia;
    const auto result = ProcessFilter::apply(processes, options);

    REQUIRE(result.size() == 2);
    CHECK(result[0].name == "Spotify");
    CHECK(result[1].name == "Spotify Helper (Renderer)");
}

TEST_CASE("ProcessFilter hideBackgroundProcesses combines with category via AND", "[ProcessFilter]")
{
    const auto processes = makeFixtureProcesses();

    ProcessFilterOptions options;
    options.category = ProcessCategory::MusicAndMedia;
    options.hideBackgroundProcesses = true;
    const auto result = ProcessFilter::apply(processes, options);

    REQUIRE(result.size() == 1);
    CHECK(result[0].name == "Spotify");
}

TEST_CASE("ProcessFilter always excludes the current process's own PID", "[ProcessFilter]")
{
    auto processes = makeFixtureProcesses();
    processes.push_back(makeProcess(ProcessFilter::getCurrentProcessID(), "This Test Binary", "/path/to/test/binary", true));

    const auto result = ProcessFilter::apply(processes, {});

    for (const auto& process : result)
        CHECK(process.processID != ProcessFilter::getCurrentProcessID());
}
