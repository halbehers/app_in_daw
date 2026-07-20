#include <catch2/catch_test_macros.hpp>

#include "Capture/SelectedProcessCapture.h"

TEST_CASE("SelectedProcessCapture reports no target process ID until one is set", "[SelectedProcessCapture]")
{
    acdsp::AudioCapture destination;
    capture::SelectedProcessCapture selectedCapture(destination);

    CHECK(selectedCapture.getTargetProcessID() == 0);
}

TEST_CASE("SelectedProcessCapture returns the target PID once set", "[SelectedProcessCapture]")
{
    acdsp::AudioCapture destination;
    capture::SelectedProcessCapture selectedCapture(destination);

    selectedCapture.setTargetProcessID(4242);
    CHECK(selectedCapture.getTargetProcessID() == 4242);
}

TEST_CASE("SelectedProcessCapture can retarget while already capturing without crashing", "[SelectedProcessCapture]")
{
    acdsp::AudioCapture destination;
    capture::SelectedProcessCapture selectedCapture(destination);

    selectedCapture.setTargetProcessID(1);
    REQUIRE_NOTHROW(selectedCapture.startCapture());

    selectedCapture.setTargetProcessID(2);
    REQUIRE_NOTHROW(selectedCapture.startCapture());

    CHECK(selectedCapture.getTargetProcessID() == 2);

    REQUIRE_NOTHROW(selectedCapture.stopCapture());
}
