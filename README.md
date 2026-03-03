# JUCE VST3/AU Plugin Template

A production-ready template for building cross-platform audio plugins using **JUCE 8.0.4**, **C++20**, and **CMake**. Includes a Python test harness powered by Spotify's [pedalboard](https://github.com/spotify/pedalboard).

## What's included

- **Plugin scaffold** — PluginProcessor, PluginEditor, example DSP module with APVTS parameter management
- **CMake build** — JUCE fetched via FetchContent (no submodules), universal macOS binary, auto-install after build
- **Test suite** — 4 tiers of deterministic tests using pedalboard to load the actual VST3 binary
- **CI/CD** — GitHub Actions for macOS (universal) + Windows builds

## Quick start

```bash
# 1. Clone and rename
gh repo create my-awesome-plugin --template furmanlukasz/juce-vst-template --private --clone
cd my-awesome-plugin

# 2. Customise plugin identity (search for "TODO" in plugin/CMakeLists.txt)
#    - PRODUCT_NAME, COMPANY_NAME, BUNDLE_ID
#    - PLUGIN_MANUFACTURER_CODE, PLUGIN_CODE

# 3. Build
cd plugin
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)"
cmake --build build --config Release

# 4. Test
cd ..
pip install pedalboard pytest scipy numpy
# Update tests/conftest.py line 19: VST3_PATH to match your PRODUCT_NAME
pytest tests/ -v
```

## Project structure

```
plugin/
  CMakeLists.txt              # JUCE 8.0.4 via FetchContent
  src/
    PluginProcessor.cpp/h     # Audio processing + parameters
    PluginEditor.cpp/h        # GUI
    dsp/
      GainProcessor.h         # Example DSP module (replace with yours)
tests/
  conftest.py                 # Plugin loader + signal fixtures
  test_plugin_basic.py        # Tier 1: load, bypass, silence, safety
  test_plugin_edge_cases.py   # Tier 3: sample rates, block sizes, extremes
  test_plugin_quality_gates.py # Tier 4: release criteria
.github/workflows/
  build.yml                   # CI: macOS + Windows
```

## Adding DSP modules

1. Create `plugin/src/dsp/YourModule.h` (and optionally `.cpp`)
2. Add it to `PLUGIN_SOURCES` in `plugin/CMakeLists.txt`
3. Include and use it in `PluginProcessor.h/cpp`
4. Add corresponding tests in `tests/`

## Test tiers

| Tier | File | Purpose |
|------|------|---------|
| 1 | `test_plugin_basic.py` | Plugin loads, bypass works, no NaN/Inf |
| 2 | *(add your own)* | Feature-specific accuracy tests |
| 3 | `test_plugin_edge_cases.py` | Extreme params, sample rates, block sizes |
| 4 | `test_plugin_quality_gates.py` | Release gates: SNR, ceiling, stereo balance |

## Prerequisites

- CMake 3.22+
- C++20 compiler (Xcode CLT on macOS, MSVC on Windows)
- Python 3.11+ (for tests)
- Git

## License

MIT
