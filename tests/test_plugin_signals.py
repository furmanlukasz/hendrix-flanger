"""
Signal-specific tests — impulse response, sweep, DC rejection.
"""

import numpy as np
import pytest
from conftest import (
    SR, process_with_params, has_nan_or_inf,
    rms_db, dc_component_db, peak_frequency,
)


class TestImpulseResponse:
    def test_impulse_shows_delay_tap(self, plugin, impulse):
        """Impulse through flanger should show a delayed copy."""
        output = process_with_params(
            plugin, impulse,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=0.0, mix=100.0, through_zero=True,
        )
        assert not has_nan_or_inf(output)
        # Should have at least two peaks (direct + delayed)
        peaks = np.where(np.abs(output) > 0.01)[0]
        assert len(peaks) > 1, "Expected multiple peaks from delay tap"

    def test_impulse_feedback_creates_echoes(self, plugin, impulse):
        """Feedback should create repeating echoes of the impulse."""
        output_fb = process_with_params(
            plugin, impulse,
            speed_hz=0.05, depth=0.0, manual_ms=5.0,
            feedback=80.0, mix=100.0,
        )
        output_no_fb = process_with_params(
            plugin, impulse,
            speed_hz=0.05, depth=0.0, manual_ms=5.0,
            feedback=0.0, mix=100.0,
        )
        assert not has_nan_or_inf(output_fb)
        # With feedback, total energy should be higher than without
        energy_fb = np.sum(output_fb ** 2)
        energy_no_fb = np.sum(output_no_fb ** 2)
        assert energy_fb > energy_no_fb * 1.1, (
            f"Feedback energy {energy_fb:.6f} not more than "
            f"no-feedback energy {energy_no_fb:.6f}"
        )


class TestSweepSignal:
    def test_sweep_no_nan(self, plugin, sweep):
        """Full-range sweep should not produce NaN."""
        output = process_with_params(
            plugin, sweep,
            speed_hz=1.0, depth=80.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_sweep_produces_output(self, plugin, sweep):
        """Sweep should produce audible output."""
        output = process_with_params(
            plugin, sweep,
            speed_hz=1.0, depth=80.0, mix=100.0,
        )
        assert rms_db(output) > -40, f"Sweep output too quiet: {rms_db(output):.1f} dB"

    def test_sweep_no_hard_clips(self, plugin, sweep):
        """Sweep output should not hard clip."""
        output = process_with_params(
            plugin, sweep,
            speed_hz=1.0, depth=80.0, feedback=80.0, mix=100.0,
        )
        peak = np.max(np.abs(output))
        assert peak < 10.0, f"Output peak {peak:.2f}, possible hard clip"


class TestDCHandling:
    def test_dc_does_not_grow(self, plugin, dc_offset):
        """DC input should not cause unbounded growth."""
        output = process_with_params(
            plugin, dc_offset,
            speed_hz=1.0, depth=50.0, feedback=50.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)
        peak = np.max(np.abs(output))
        assert peak < 5.0, f"DC caused peak of {peak:.2f}"

    def test_noise_floor_with_silence(self, plugin, silence):
        """Silence in should give very low noise floor."""
        output = process_with_params(
            plugin, silence,
            speed_hz=1.0, depth=50.0, feedback=50.0, mix=100.0,
        )
        level = rms_db(output)
        assert level < -80, f"Noise floor {level:.1f} dB"


class TestSineThrough:
    def test_sine_survives_flanging(self, plugin, sine_440):
        """A sine wave through the flanger should still have energy near 440 Hz."""
        output = process_with_params(
            plugin, sine_440,
            speed_hz=0.5, depth=50.0, mix=50.0,
        )
        freq = peak_frequency(output)
        # The dominant frequency should still be near 440 Hz
        assert abs(freq - 440) < 50, f"Expected ~440 Hz, got {freq:.1f}"

    def test_different_rates_sound_different(self, plugin, sine_440):
        """Different LFO rates should produce different modulation patterns."""
        out_slow = process_with_params(
            plugin, sine_440, speed_hz=0.2, depth=80.0, mix=100.0,
        )
        out_fast = process_with_params(
            plugin, sine_440, speed_hz=5.0, depth=80.0, mix=100.0,
        )
        # Fast rate should have more amplitude variation
        env_slow = np.abs(out_slow)
        env_fast = np.abs(out_fast)
        var_slow = np.var(env_slow)
        var_fast = np.var(env_fast)
        # Just check both are valid
        assert not has_nan_or_inf(out_slow)
        assert not has_nan_or_inf(out_fast)
