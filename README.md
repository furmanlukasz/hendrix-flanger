# Hendrix Flanger

A through-zero flanger VST3/AU plugin inspired by the tape flanging techniques used on classic Jimi Hendrix recordings. Built with **JUCE 8.0.4**, **C++20**, and **CMake**.

## The Effect

Through-zero flanging replicates the studio tape technique where two identical tape recordings are played simultaneously with one slightly varied in speed. When the delayed signal crosses through zero delay, deep phase cancellations produce the characteristic "jet engine" sweep heard on tracks like *Bold as Love* and *Voodoo Child (Slight Return)*.

## Features

- **Through-Zero Flanging** — sweeps delay through zero for deep cancellations
- **Positive & Negative Feedback** — metallic resonance or hollow Hendrix-style notches
- **Stereo Spread** — LFO phase offset between L/R for wide stereo movement
- **Envelope Follower** — dynamic modulation from input signal level
- **3 LFO Shapes** — Sine, Triangle, Sample & Hold

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Speed | 0.05–10 Hz | 0.5 Hz | LFO rate |
| Depth | 0–100% | 50% | Sweep width |
| Manual | 0–10 ms | 3 ms | Base delay |
| Feedback | -95 to +95% | 30% | Regeneration |
| Stereo | 0–180 deg | 90 deg | L/R phase offset |
| Through Zero | on/off | on | TZF mode |
| Envelope | 0–100% | 0% | Env follower depth |
| LFO Shape | Sine/Tri/S&H | Sine | Waveform |
| Mix | 0–100% | 50% | Dry/wet blend |

## Build

```bash
cd plugin
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The VST3 and AU are auto-installed to your system plugin folders.

## Test

```bash
pip install pedalboard pytest scipy numpy
pytest tests/ -v
```

Tests load the **actual built VST3** via Spotify's pedalboard — no mocks.

## Project Structure

```
plugin/
  CMakeLists.txt
  src/
    PluginProcessor.cpp/h
    PluginEditor.cpp/h
    dsp/
      DelayLine.h
      LFO.h
      EnvelopeFollower.h
      ThroughZeroFlanger.h
tests/
  conftest.py
  test_plugin_basic.py
  test_plugin_flanger.py
  test_plugin_signals.py
  test_plugin_edge_cases.py
  test_plugin_quality_gates.py
```

## License

MIT
