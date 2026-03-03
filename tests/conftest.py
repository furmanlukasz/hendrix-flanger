"""
Shared fixtures for VST3 plugin tests.

All signal fixtures are deterministic (exact math or seeded RNG).
The plugin fixture loads the actual built VST3 binary via pedalboard.
"""

import os
import pytest
import numpy as np


# ---------------------------------------------------------------------------
# Constants — TODO: update VST3_PATH to match your PRODUCT_NAME
# ---------------------------------------------------------------------------

SR = 44100
DURATION = 1.0
VST3_PATH = os.path.expanduser(
    "~/Library/Audio/Plug-Ins/VST3/My Plugin.vst3"
)


# ---------------------------------------------------------------------------
# Plugin fixtures
# ---------------------------------------------------------------------------


@pytest.fixture(scope="session")
def plugin():
    """Load the built VST3 plugin (shared across all tests)."""
    if not os.path.exists(VST3_PATH):
        pytest.skip(
            f"VST3 not found at {VST3_PATH} — build first with: "
            "cd plugin && cmake -B build -DCMAKE_BUILD_TYPE=Release && "
            "cmake --build build --config Release"
        )
    from pedalboard import load_plugin

    return load_plugin(VST3_PATH)


@pytest.fixture()
def fresh_plugin():
    """Load a fresh plugin instance (no shared state between tests)."""
    if not os.path.exists(VST3_PATH):
        pytest.skip("VST3 not built")
    from pedalboard import load_plugin

    return load_plugin(VST3_PATH)


# ---------------------------------------------------------------------------
# Signal fixtures (all deterministic)
# ---------------------------------------------------------------------------


@pytest.fixture()
def sine_440():
    """440 Hz sine wave, 1 second."""
    t = np.arange(int(SR * DURATION)) / SR
    return (np.sin(2 * np.pi * 440 * t)).astype(np.float32)


@pytest.fixture()
def sine_1000():
    """1000 Hz sine wave, 1 second."""
    t = np.arange(int(SR * DURATION)) / SR
    return (np.sin(2 * np.pi * 1000 * t)).astype(np.float32)


@pytest.fixture()
def impulse():
    """Single-sample unit impulse at t=0."""
    sig = np.zeros(int(SR * DURATION), dtype=np.float32)
    sig[0] = 1.0
    return sig


@pytest.fixture()
def silence():
    """1 second of silence."""
    return np.zeros(int(SR * DURATION), dtype=np.float32)


@pytest.fixture()
def white_noise():
    """Seeded white noise (seed=42), 1 second."""
    rng = np.random.default_rng(42)
    return rng.standard_normal(int(SR * DURATION)).astype(np.float32) * 0.5


@pytest.fixture()
def dc_offset():
    """Constant DC offset of 0.5 for 1 second."""
    return np.full(int(SR * DURATION), 0.5, dtype=np.float32)


# ---------------------------------------------------------------------------
# Helper functions
# ---------------------------------------------------------------------------


def peak_frequency(signal: np.ndarray, sr: int = SR) -> float:
    """Find dominant frequency via FFT."""
    windowed = signal * np.hanning(len(signal))
    spectrum = np.abs(np.fft.rfft(windowed))
    freqs = np.fft.rfftfreq(len(signal), 1.0 / sr)
    spectrum[0] = 0  # ignore DC
    return float(freqs[np.argmax(spectrum)])


def rms_db(signal: np.ndarray) -> float:
    """RMS level in dB (relative to 1.0)."""
    rms = np.sqrt(np.mean(signal.astype(np.float64) ** 2))
    if rms < 1e-20:
        return -200.0
    return float(20 * np.log10(rms))


def has_nan_or_inf(signal: np.ndarray) -> bool:
    """Check for NaN or Inf values."""
    return bool(np.any(np.isnan(signal)) or np.any(np.isinf(signal)))


def process_with_params(plugin, signal: np.ndarray, sr: int = SR, **params) -> np.ndarray:
    """Process a signal through the plugin with given parameters.

    Handles mono->stereo conversion. Returns first channel as 1D array.
    """
    plugin.reset()

    for name, value in params.items():
        setattr(plugin, name, value)

    if signal.ndim == 1:
        audio = signal[np.newaxis, :]
    else:
        audio = signal

    output = plugin.process(audio, sample_rate=float(sr))

    if output.ndim == 2:
        return output[0]
    return output
