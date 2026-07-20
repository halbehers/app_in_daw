add_executable(${PROJECT_NAME}_Tests
        AppSettingsTests.cpp
        ParametersTests.cpp
        PluginProcessorLatencyTests.cpp
        PluginProcessorProcessCaptureTests.cpp
        PluginStatePersistenceTests.cpp
        ProcessCategoryMatcherTests.cpp
        ProcessFilterTests.cpp
        ProcessListIntegrationTests.cpp
        SelectedProcessCaptureTests.cpp
        main.cpp
)

target_link_libraries(${PROJECT_NAME}_Tests PRIVATE
        ${PROJECT_NAME}
        Catch2::Catch2
        juce::juce_recommended_warning_flags
)

# Catch2's CMake extras (Catch.cmake, providing catch_discover_tests) aren't on CMAKE_MODULE_PATH
# automatically when Catch2 is fetched via CPM/FetchContent as source, unlike an installed package.
list(APPEND CMAKE_MODULE_PATH "${CPM_PACKAGE_CATCH2_SOURCE_DIR}/extras")

include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME}_Tests)
