# App In DAW

**App In DAW** is a macOS audio plugin by Nierika that captures the audio of another running
application — a browser tab, Spotify, a video call, a game — and routes it into your DAW as the
plugin's own output. Pick the source app from an Activity-Monitor-style process list, and its
audio becomes a track you can record, process, or mix like any other input.

Available as Standalone, AU, AUv3, and VST3.

## Features

- **Pick any running process** from a searchable, filterable, sortable table (name, category, PID).
- **Category filtering** — apps are automatically grouped into Music & Media, Browsers,
  Communication & Meetings, and Creative & DAW, with an "All processes / Apps only" toggle to hide
  background helpers.
- **Customizable categorization** — extend the built-in app list yourself without recompiling, see
  [Customizing process categories](#customizing-process-categories) below.
- **Remembers your last captured process** and tries to reacquire it automatically when your DAW
  project reopens.
- **Latency monitor**, output trim (0 / -6 / -12 dB), and light/dark theme.
- Localized in English, French, Spanish, German, Italian, and Portuguese.

## Requirements

- macOS (process audio capture and plugin validation are both macOS-only)
- CMake ≥ 3.22, [Ninja](https://ninja-build.org/)
- A C++20 compiler (Xcode command line tools)

## Building

Dependencies — [JUCE](https://github.com/juce-framework/JUCE) 8.0.14, [Catch2](https://github.com/catchorg/Catch2),
and Nierika's `nierika_dsp`/`audio_capture_dsp` modules — are fetched automatically via
[CPM](https://github.com/cpm-cmake/CPM.cmake) on first configure.

```sh
cmake --preset default          # configure (Debug, Ninja)
cmake --build --preset default  # build
```

Built plugin bundles land in `build/AppInDAW_artefacts/Debug/{Standalone,AU,VST3}`. For a Release
build:

```sh
cmake --preset release
cmake --build --preset release
```

Xcode and Visual Studio project generation is available via the `Xcode`/`vs` presets.

## Testing

```sh
ctest --test-dir build
```

Runs the Catch2 unit test suite plus an end-to-end [pluginval](https://github.com/Tracktion/pluginval)
validation pass against the built AU (parameter automation, state save/restore, thread safety,
repeated editor open/close). See `build/Tests/AppInDAW_Tests --help` for running a subset of tests
by name or tag.

## Customizing process categories

Which category an app falls into (Music & Media, Browsers, Communication & Meetings, Creative &
DAW) is decided by matching its name and executable path against a list of known apps. The app
ships with a curated default list, but you can extend it yourself: on first launch, App In DAW
creates an editable template at

```
~/Library/Nierika/App In DAW/process_categories.json
```

Add your own app names/paths to any category — matching is a case-insensitive substring check, so
partial names work (e.g. `"Reason"` matches `"Reason 12.app"`). Your entries are merged on top of
the built-in list, so you never need to repeat what's already covered. Restart the plugin/DAW for
changes to take effect.

## Project layout

- `Code/Include`, `Code/Source` — plugin source, mirrored header/implementation layout.
- `Assets/Languages` — localization strings (`.lang` files) and `Assets/process_categories.json` —
  the bundled default app-category list.
- `Tests` — Catch2 unit tests.
- `CMake` — build configuration helpers (dependency fetching, compiler warnings, `pluginval`
  integration).
- `Libs` — CPM-fetched dependencies (JUCE, and Nierika's own modules unless a local checkout is
  configured — see `CMakeLists.txt`).

For a deeper architectural overview, see [CLAUDE.md](CLAUDE.md).
