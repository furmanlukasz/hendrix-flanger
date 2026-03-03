"""
Basic smoke tests — does the plugin load, respond to parameters, and not crash?
"""

import numpy as np
import pytest
from conftest import SR, process_with_params, has_nan_or_inf, rms_db, energy_ratio_db


class TestPluginLoads:
    def test_plugin_loads(self, plugin):
        """VST3 binary loads without crash."""
        assert plugin is not None

    def test_plugin_has_speed(self, plugin):
        assert hasattr(plugin, "speed_hz")

    def test_plugin_has_depth(self, plugin):
        assert hasattr(plugin, "depth")

    def test_plugin_has_manual(self, plugin):
        assert hasattr(plugin, "manual_ms")

    def test_plugin_has_feedback(self, plugin):
        assert hasattr(plugin, "feedback")

    def test_plugin_has_stereo(self, plugin):
        assert hasattr(plugin, "stereo")

    def test_plugin_has_through_zero(self, plugin):
        assert hasattr(plugin, "through_zero")

    def test_plugin_has_envelope(self, plugin):
        assert hasattr(plugin, "envelope")

    def test_plugin_has_lfo_shape(self, plugin):
        assert hasattr(plugin, "lfo_shape")

    def test_plugin_has_mix(self, plugin):
        assert hasattr(plugin, "mix")


class TestBasicBehavior:
    def test_silence_in_silence_out(self, plugin, silence):
        """Silent input should produce near-silent output."""
        output = process_with_params(plugin, silence, mix=100.0)
        level = rms_db(output)
        assert level < -80, f"Output level {level:.1f} dB, expected < -80 dB"

    def test_no_nan_on_sine(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, mix=100.0)
        assert not has_nan_or_inf(output)

    def test_no_nan_on_impulse(self, plugin, impulse):
        output = process_with_params(plugin, impulse, mix=100.0)
        assert not has_nan_or_inf(output)

    def test_no_nan_on_noise(self, plugin, white_noise):
        output = process_with_params(plugin, white_noise, mix=100.0)
        assert not has_nan_or_inf(output)

    def test_output_bounded(self, plugin, sine_440):
        """Output peak should not exceed +12 dBFS."""
        output = process_with_params(
            plugin, sine_440, feedback=95.0, depth=100.0, mix=100.0
        )
        peak_db = 20 * np.log10(np.max(np.abs(output)) + 1e-20)
        assert peak_db < 12.0, f"Peak at {peak_db:.1f} dBFS"

    def test_dry_wet_zero_is_dry(self, plugin, sine_440):
        """Mix at 0% should pass dry signal unchanged."""
        output = process_with_params(plugin, sine_440, mix=0.0)
        corr = np.corrcoef(sine_440[:len(output)], output[:len(sine_440)])[0, 1]
        assert corr > 0.99, f"Correlation {corr:.4f}, expected > 0.99"

    def test_reset_determinism(self, plugin, sine_440):
        """Processing the same signal twice after reset gives same result."""
        out1 = process_with_params(
            plugin, sine_440, speed_hz=1.0, depth=50.0, mix=100.0
        )
        out2 = process_with_params(
            plugin, sine_440, speed_hz=1.0, depth=50.0, mix=100.0
        )
        assert np.allclose(out1, out2, atol=1e-6), "Non-deterministic output after reset"

    def test_energy_preserved_roughly(self, plugin, sine_440):
        """Flanger should not wildly change energy (within 6 dB)."""
        output = process_with_params(
            plugin, sine_440, depth=50.0, feedback=0.0, mix=50.0
        )
        ratio = energy_ratio_db(output, sine_440)
        assert abs(ratio) < 6, f"Energy ratio {ratio:.1f} dB, expected within 6 dB"
