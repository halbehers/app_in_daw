# App In DAW

**App In DAW** is a macOS & Windows audio plugin by Nierika that captures the audio of another running
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

- macOS ≥ 14.5 or Windows ≥ 10 (2020)
- CMake ≥ 3.22, [Ninja](https://ninja-build.org/)
- A C++20 compiler (Xcode command line tools)

## Building

Dependencies — [JUCE](https://github.com/juce-framework/JUCE) 8.0.14, [Catch2](https://github.com/catchorg/Catch2),
and Nierika's `nierika_dsp`/`audio_capture_dsp` modules — are fetched automatically via
[CPM](https://github.com/cpm-cmake/CPM.cmake) on first configure.

```sh
cmake --workflow --preset default  # configure (first run/whenever CMakeLists.txt changes) + build
```

`--workflow` always does the right thing whether `build/` already exists or not (a fresh clone, or
after deleting it) - if you'd rather configure and build as separate steps (e.g. to build
repeatedly without reconfiguring), that still works too, as long as `build/` already exists:

```sh
cmake --preset default          # configure (Debug, Ninja) - only needed once, or after CMakeLists.txt changes
cmake --build --preset default  # build
```

Built plugin bundles land in `build/AppInDAW_artefacts/Debug/{Standalone,AU,VST3}`. For a Release
build (also produces an installer - see below):

```sh
cmake --workflow --preset release
```

Xcode and Visual Studio project generation is available via the `Xcode`/`vs` presets.

### Installers

A Release build also packages an installer automatically: `release-build/Packaging/App In DAW-<version>-macOS.pkg`
(AU + VST3, ad-hoc signed unless real Developer ID credentials are configured in the environment)
on macOS, or `release-build\Packaging\App In DAW-<version>-Windows.exe` (VST3, requires
[Inno Setup](https://jrsoftware.org/isdl.php)'s `iscc` on `PATH`) on Windows.

## Testing

```sh
ctest --test-dir build
```

Runs the Catch2 unit test suite plus an end-to-end [pluginval](https://github.com/Tracktion/pluginval)
validation pass against the built AU (parameter automation, state save/restore, thread safety,
repeated editor open/close). See `build/Tests/AppInDAW_Tests --help` for running a subset of tests
by name or tag.

## Continuous integration

`.github/workflows/build_and_test.yml` runs on GitHub Actions:

- **When it runs**: on every push to any branch, on every pull request (a same-repo PR is skipped
  since the matching push event already covers it — only PRs from forks trigger separately), and
  on demand via the Actions tab's "Run workflow" button (`workflow_dispatch`).
- **What it does**: builds a matrix of macOS (universal `arm64`+`x86_64`) and Windows, each of
  which configures and builds a Release, runs `ctest` (the Catch2 suite plus the `pluginval`
  validation pass), then uploads the resulting installer as a workflow artifact, downloadable from
  that run's summary page.
- **Artifact generation/signing**: the installer itself is produced by the *same* Release build
  described in [Installers](#installers) above (`CMake/packaging.cmake`) — CI doesn't package
  anything separately or differently from a local `cmake --workflow --preset release`. Signing is
  the only thing that differs by environment: it's ad-hoc/unsigned unless the corresponding
  repository secrets are configured, so the workflow always produces something installable even
  without a paid signing setup:
  - macOS: imports a Developer ID Application/Installer certificate from the `DEV_ID_APP_CERT` /
    `DEV_ID_APP_PASSWORD` / `DEV_ID_INSTALLER_CERT` / `DEV_ID_INSTALLER_PASSWORD` secrets, then
    notarizes and staples the `.pkg` if `NOTARIZATION_USERNAME` / `NOTARIZATION_PASSWORD` /
    `TEAM_ID` are also set.
  - Windows: signs the `.exe` via Azure Artifact Signing if `AZURE_TENANT_ID` (and the other
    `AZURE_*` secrets) are set.
- **Releases**: pushing a tag matching `v*` (e.g. `v1.2.0`) additionally runs the `release` job,
  which downloads both installers from that run and attaches them to a new pre-release GitHub
  Release — no separate manual publish step needed.

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


---

## Developers

Sebastien Halbeher (`halbehers`) - see [LICENSE](LICENSE).
