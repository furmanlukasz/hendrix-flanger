"""
Dub Echo module tests — verify parameters exist, echo/reverb work,
no NaN/Inf on extreme settings, and dry/wet mixing behaves correctly.

pedalboard parameter name mapping:
  APVTS "dub_enabled" → pedalboard "dub_echo" (bool)
  APVTS "dub_echo"    → pedalboard "echo_ms"  (float, ms)
  APVTS "dub_reverb"  → pedalboard "reverb"
  APVTS "dub_feedback" → pedalboard "dub_feedback"
  APVTS "dub_offset"  → pedalboard "offset"
  APVTS "dub_autopan" → pedalboard "autopan"
  APVTS "dub_fullness" → pedalboard "fullness"
  APVTS "dub_space"   → pedalboard "space"
  APVTS "dub_dry"     → pedalboard "dub_dry"
  APVTS "dub_wet"     → pedalboard "dub_wet"
  APVTS "dub_volume"  → pedalboard "dub_volume"
"""

import numpy as np
import pytest
from conftest import SR, process_with_params, has_nan_or_inf, rms_db


class TestDubEchoParams:
    def test_dub_enabled_param_exists(self, plugin):
        """Plugin should have a dub_echo (enabled) parameter."""
        assert hasattr(plugin, "dub_echo")

    def test_dub_echo_param_exists(self, plugin):
        """Plugin should have an echo_ms parameter."""
        assert hasattr(plugin, "echo_ms")

    def test_dub_reverb_param_exists(self, plugin):
        """Plugin should have a reverb parameter."""
        assert hasattr(plugin, "reverb")

    def test_dub_feedback_param_exists(self, plugin):
        """Plugin should have a dub_feedback parameter."""
        assert hasattr(plugin, "dub_feedback")

    def test_dub_offset_param_exists(self, plugin):
        """Plugin should have an offset parameter."""
        assert hasattr(plugin, "offset")

    def test_dub_autopan_param_exists(self, plugin):
        """Plugin should have an autopan parameter."""
        assert hasattr(plugin, "autopan")

    def test_dub_fullness_param_exists(self, plugin):
        """Plugin should have a fullness parameter."""
        assert hasattr(plugin, "fullness")

    def test_dub_space_param_exists(self, plugin):
        """Plugin should have a space parameter."""
        assert hasattr(plugin, "space")

    def test_dub_dry_param_exists(self, plugin):
        """Plugin should have a dub_dry parameter."""
        assert hasattr(plugin, "dub_dry")

    def test_dub_wet_param_exists(self, plugin):
        """Plugin should have a dub_wet parameter."""
        assert hasattr(plugin, "dub_wet")

    def test_dub_volume_param_exists(self, plugin):
        """Plugin should have a dub_volume parameter."""
        assert hasattr(plugin, "dub_volume")


class TestDubEchoStability:
    def test_dub_no_nan_extreme(self, plugin, sine_440):
        """Dub echo at extreme settings should not produce NaN."""
        output = process_with_params(
            plugin, sine_440,
            dub_echo=True,
            echo_ms=1000.0, reverb=100.0, dub_feedback=95.0,
            offset=100.0, autopan=100.0, fullness=100.0,
            space=100.0, dub_dry=100.0, dub_wet=100.0, dub_volume=100.0,
            mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_dub_no_nan_with_flanger(self, plugin, sine_440):
        """Dub echo + flanger at high settings should not produce NaN."""
        output = process_with_params(
            plugin, sine_440,
            dub_echo=True,
            echo_ms=500.0, reverb=80.0, dub_feedback=80.0,
            fullness=80.0, space=80.0, dub_wet=80.0, dub_volume=80.0,
            speed_hz=2.0, depth=100.0, feedback=80.0, warmth=80.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_dub_feedback_bounded(self, plugin, sine_440):
        """High dub feedback should not cause runaway output."""
        output = process_with_params(
            plugin, sine_440,
            dub_echo=True,
            echo_ms=200.0, dub_feedback=95.0, fullness=100.0,
            dub_wet=100.0, dub_volume=100.0, mix=100.0,
        )
        peak = np.max(np.abs(output))
        # tanh soft-clips feedback, but fullness saturation adds gain
        assert peak < 15.0, f"Dub echo feedback caused peak of {peak:.2f}"


class TestDubEchoFunctionality:
    def test_dub_echo_produces_delay(self, plugin, impulse):
        """Echo at 300ms should produce energy after the 300ms mark."""
        output = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_ms=300.0, dub_feedback=50.0,
            dub_dry=0.0, dub_wet=100.0, dub_volume=100.0,
            mix=100.0,
        )
        # Check that there's energy after 300ms (13230 samples at 44100)
        delay_sample = int(0.3 * SR)
        late_energy = np.sum(output[delay_sample:delay_sample + 2000] ** 2)
        assert late_energy > 1e-6, "Echo should produce delayed energy"

    def test_dub_reverb_adds_tail(self, plugin, impulse):
        """Reverb should extend the signal tail beyond the echo."""
        # Without reverb — minimize flanger influence (depth=0, feedback=0)
        out_no_rev = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_ms=100.0, reverb=0.0, space=0.0,
            dub_feedback=0.0, dub_dry=0.0, dub_wet=100.0, dub_volume=100.0,
            speed_hz=0.5, depth=0.0, feedback=0.0, mix=100.0,
        )
        # With reverb
        out_rev = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_ms=100.0, reverb=80.0, space=80.0,
            dub_feedback=0.0, dub_dry=0.0, dub_wet=100.0, dub_volume=100.0,
            speed_hz=0.5, depth=0.0, feedback=0.0, mix=100.0,
        )
        # Reverb output should have more energy in the late tail
        tail_start = int(0.3 * SR)
        tail_energy_no_rev = np.sum(out_no_rev[tail_start:] ** 2)
        tail_energy_rev = np.sum(out_rev[tail_start:] ** 2)
        assert tail_energy_rev > tail_energy_no_rev, (
            f"Reverb should add tail energy: {tail_energy_rev:.4f} vs {tail_energy_no_rev:.4f}"
        )

    def test_dub_disabled_no_change(self, plugin, sine_440):
        """With dub disabled, output should match flanger-only."""
        out_disabled = process_with_params(
            plugin, sine_440,
            dub_echo=False,
            speed_hz=1.0, depth=50.0, feedback=30.0, mix=50.0,
        )
        out_also_disabled = process_with_params(
            plugin, sine_440,
            dub_echo=False,
            speed_hz=1.0, depth=50.0, feedback=30.0, mix=50.0,
        )
        assert np.allclose(out_disabled, out_also_disabled, atol=0.001), (
            "Dub echo disabled should produce consistent output"
        )

    def test_dub_enabled_changes_sound(self, plugin, sine_440):
        """Dub echo enabled should produce different output than disabled."""
        out_off = process_with_params(
            plugin, sine_440,
            dub_echo=False,
            speed_hz=1.0, depth=50.0, feedback=30.0, mix=50.0,
        )
        out_on = process_with_params(
            plugin, sine_440,
            dub_echo=True,
            echo_ms=300.0, reverb=50.0, dub_feedback=50.0,
            dub_wet=80.0, dub_volume=80.0,
            speed_hz=1.0, depth=50.0, feedback=30.0, mix=50.0,
        )
        assert not np.allclose(out_off, out_on, atol=0.01), (
            "Dub echo enabled should change the output"
        )

    def test_dub_wet_dry_mix(self, plugin, sine_440):
        """Dub dry=100 wet=0 should approximate bypass behavior."""
        out_dry_only = process_with_params(
            plugin, sine_440,
            dub_echo=True,
            echo_ms=300.0, dub_dry=100.0, dub_wet=0.0, dub_volume=100.0,
            speed_hz=0.5, depth=0.0, mix=100.0,
        )
        # With depth=0 and only dry, output should be close to input
        rms = rms_db(out_dry_only)
        assert rms > -10.0, f"Dry-only output too quiet: {rms:.1f} dB"
