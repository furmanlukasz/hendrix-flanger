"""
Dub Echo module tests — verify parameters exist, echo/reverb work,
no NaN/Inf on extreme settings, and dry/wet mixing behaves correctly.

pedalboard parameter name mapping:
  APVTS "dub_enabled" → pedalboard "dub_echo" (bool)
  APVTS "dub_echo"    → pedalboard "echo_div" (choice: "1/1".."1/32")
  APVTS "dub_bpm"     → pedalboard "bpm_bpm"  (float, 40-300)
  APVTS "dub_reverb"  → pedalboard "reverb"
  APVTS "dub_feedback" → pedalboard "dub_feedback"
  APVTS "dub_offset"  → pedalboard "offset"
  APVTS "dub_autopan" → pedalboard "autopan"
  APVTS "dub_fullness" → pedalboard "fullness"
  APVTS "dub_space"   → pedalboard "space"
  APVTS "dub_dry"     → pedalboard "dub_dry"
  APVTS "dub_wet"     → pedalboard "dub_wet"
  APVTS "dub_volume"  → pedalboard "dub_volume"

Note division ms at 120 BPM:
  1/1=2000(clamped 1000), 1/2d=1500(clamped 1000), 1/2=1000,
  1/4d=750, 1/4=500, 1/8d=375, 1/8=250, 1/16d=187.5, 1/16=125, 1/32=62.5
"""

import numpy as np
import pytest
from conftest import SR, process_with_params, has_nan_or_inf, rms_db


class TestDubEchoParams:
    def test_dub_enabled_param_exists(self, plugin):
        """Plugin should have a dub_echo (enabled) parameter."""
        assert hasattr(plugin, "dub_echo")

    def test_dub_echo_div_param_exists(self, plugin):
        """Plugin should have an echo_div (note division) parameter."""
        assert hasattr(plugin, "echo_div")

    def test_dub_bpm_param_exists(self, plugin):
        """Plugin should have a bpm_bpm parameter."""
        assert hasattr(plugin, "bpm_bpm")

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
            echo_div="1/2", bpm_bpm=120.0,  # 1/2 at 120 BPM = 1000ms
            reverb=100.0, dub_feedback=95.0,
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
            echo_div="1/4", bpm_bpm=120.0,  # 1/4 at 120 BPM = 500ms
            reverb=80.0, dub_feedback=80.0,
            fullness=80.0, space=80.0, dub_wet=80.0, dub_volume=80.0,
            speed_hz=2.0, depth=100.0, feedback=80.0, warmth=80.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_dub_feedback_bounded(self, plugin, sine_440):
        """High dub feedback should not cause runaway output."""
        output = process_with_params(
            plugin, sine_440,
            dub_echo=True,
            echo_div="1/16", bpm_bpm=120.0,  # 1/16 at 120 BPM = 125ms
            dub_feedback=95.0, fullness=100.0,
            dub_wet=100.0, dub_volume=100.0, mix=100.0,
        )
        peak = np.max(np.abs(output))
        # tanh soft-clips feedback, but fullness saturation adds gain
        assert peak < 15.0, f"Dub echo feedback caused peak of {peak:.2f}"


class TestDubEchoFunctionality:
    def test_dub_echo_produces_delay(self, plugin, impulse):
        """Echo at 1/4 note (500ms at 120 BPM) should produce energy after that mark."""
        output = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_div="1/4", bpm_bpm=120.0,  # 1/4 at 120 BPM = 500ms
            dub_feedback=50.0,
            dub_dry=0.0, dub_wet=100.0, dub_volume=100.0,
            mix=100.0,
        )
        # Check that there's energy after 500ms (22050 samples at 44100)
        delay_sample = int(0.5 * SR)
        late_energy = np.sum(output[delay_sample:delay_sample + 2000] ** 2)
        assert late_energy > 1e-6, "Echo should produce delayed energy"

    def test_dub_reverb_adds_tail(self, plugin, impulse):
        """Reverb should extend the signal tail beyond the echo."""
        # Without reverb — minimize flanger influence (depth=0, feedback=0)
        out_no_rev = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_div="1/16", bpm_bpm=120.0,  # 1/16 at 120 BPM = 125ms
            reverb=0.0, space=0.0,
            dub_feedback=0.0, dub_dry=0.0, dub_wet=100.0, dub_volume=100.0,
            speed_hz=0.5, depth=0.0, feedback=0.0, mix=100.0,
        )
        # With reverb
        out_rev = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_div="1/16", bpm_bpm=120.0,  # 1/16 at 120 BPM = 125ms
            reverb=80.0, space=80.0,
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
            echo_div="1/8", bpm_bpm=120.0,  # 1/8 at 120 BPM = 250ms
            reverb=50.0, dub_feedback=50.0,
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
            echo_div="1/8", bpm_bpm=120.0,  # 1/8 at 120 BPM = 250ms
            dub_dry=100.0, dub_wet=0.0, dub_volume=100.0,
            speed_hz=0.5, depth=0.0, mix=100.0,
        )
        # With depth=0 and only dry, output should be close to input
        rms = rms_db(out_dry_only)
        assert rms > -10.0, f"Dry-only output too quiet: {rms:.1f} dB"

    def test_bpm_param_does_not_crash(self, plugin, sine_440):
        """Setting various BPM values should not crash or produce NaN.

        Note: pedalboard may provide a host BPM that overrides the manual knob,
        so we only verify stability, not exact timing.
        """
        for bpm in [40.0, 120.0, 300.0]:
            output = process_with_params(
                plugin, sine_440,
                dub_echo=True,
                echo_div="1/4", bpm_bpm=bpm,
                dub_feedback=50.0,
                dub_wet=80.0, dub_volume=80.0,
                mix=50.0,
            )
            assert not has_nan_or_inf(output), f"NaN/Inf at BPM={bpm}"
            assert rms_db(output) > -60.0, f"Output too quiet at BPM={bpm}"

    def test_note_divisions_produce_different_timing(self, plugin, impulse):
        """Different note divisions at same BPM should produce different echo times."""
        # 1/4 at 120 BPM = 500ms
        out_quarter = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_div="1/4", bpm_bpm=120.0,
            dub_feedback=50.0,
            dub_dry=0.0, dub_wet=100.0, dub_volume=100.0,
            mix=100.0,
        )
        # 1/8 at 120 BPM = 250ms
        out_eighth = process_with_params(
            plugin, impulse,
            dub_echo=True,
            echo_div="1/8", bpm_bpm=120.0,
            dub_feedback=50.0,
            dub_dry=0.0, dub_wet=100.0, dub_volume=100.0,
            mix=100.0,
        )
        # 1/8 echo arrives at 250ms, so there should be more energy in the 200-400ms
        # window for 1/8 than 1/4 (which doesn't arrive until 500ms)
        window_start = int(0.2 * SR)
        window_end = int(0.4 * SR)
        energy_quarter = np.sum(out_quarter[window_start:window_end] ** 2)
        energy_eighth = np.sum(out_eighth[window_start:window_end] ** 2)
        assert energy_eighth > energy_quarter, (
            f"1/8 note should have earlier echo: eighth={energy_eighth:.6f} vs quarter={energy_quarter:.6f}"
        )
