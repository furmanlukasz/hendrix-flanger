"""
Tier 1: Basic plugin tests — loading, bypass, silence, safety.

These must ALL pass before shipping any build.
"""

import numpy as np
import pytest
from conftest import (
    SR,
    has_nan_or_inf,
    peak_frequency,
    process_with_params,
    rms_db,
)


class TestPluginLoads:
    """Verify the plugin loads and exposes expected parameters."""

    def test_plugin_loads(self, plugin):
        assert plugin is not None

    def test_plugin_has_parameters(self, plugin):
        assert len(plugin.parameters) > 0

    def test_plugin_parameter_names(self, plugin):
        """TODO: Update with your actual parameter names."""
        param_names = list(plugin.parameters.keys())
        assert "gain_db" in param_names
        assert "dry_wet" in param_names
        assert "bypass" in param_names


class TestBypass:
    """Bypass / identity behavior."""

    def test_bypass_preserves_signal(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, bypass=True)
        freq = peak_frequency(output)
        assert abs(freq - 440) < 10, f"Bypass changed frequency to {freq:.1f}"

    def test_bypass_preserves_energy(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, bypass=True)
        in_rms = rms_db(sine_440)
        out_rms = rms_db(output)
        assert abs(in_rms - out_rms) < 1.0, f"Bypass changed level by {abs(in_rms - out_rms):.1f} dB"

    def test_reset_produces_same_output(self, plugin, sine_440):
        out1 = process_with_params(plugin, sine_440, gain_db=0.0, dry_wet=100.0)
        out2 = process_with_params(plugin, sine_440, gain_db=0.0, dry_wet=100.0)
        assert np.allclose(out1, out2, atol=1e-6)


class TestSilence:
    """Silence in -> silence out."""

    def test_silence_in_silence_out(self, plugin, silence):
        output = process_with_params(plugin, silence, gain_db=0.0, dry_wet=100.0)
        level = rms_db(output)
        assert level < -80, f"Silence produced {level:.1f} dB"

    def test_noise_floor(self, plugin, silence):
        output = process_with_params(plugin, silence, gain_db=0.0, dry_wet=100.0)
        peak = np.max(np.abs(output))
        assert peak < 0.001, f"Noise floor peak: {peak:.6f}"


class TestSafety:
    """No NaN, no Inf, no explosions."""

    def test_no_nan_on_sine(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, gain_db=0.0, dry_wet=100.0)
        assert not has_nan_or_inf(output)

    def test_no_nan_on_impulse(self, plugin, impulse):
        output = process_with_params(plugin, impulse, gain_db=0.0, dry_wet=100.0)
        assert not has_nan_or_inf(output)

    def test_no_nan_on_noise(self, plugin, white_noise):
        output = process_with_params(plugin, white_noise, gain_db=0.0, dry_wet=100.0)
        assert not has_nan_or_inf(output)

    def test_output_bounded(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, gain_db=12.0, dry_wet=100.0)
        peak = np.max(np.abs(output))
        assert peak < 10.0, f"Output peak {peak:.2f} is dangerously loud"
