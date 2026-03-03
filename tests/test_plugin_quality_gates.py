"""
Quality gates — ALL must pass before shipping.

These are release criteria for the Hendrix Flanger.
"""

import numpy as np
import pytest
from conftest import (
    SR, process_with_params, process_stereo,
    has_nan_or_inf, rms_db,
)


class TestQualityGates:
    def test_gate_bypass_snr(self, plugin, sine_440):
        """Bypass (mix=0) should have SNR > 80 dB."""
        output = process_with_params(plugin, sine_440, mix=0.0)
        noise = output[:len(sine_440)] - sine_440[:len(output)]
        signal_rms = rms_db(sine_440)
        noise_rms = rms_db(noise)
        snr = signal_rms - noise_rms
        assert snr > 80, f"Bypass SNR {snr:.1f} dB, expected > 80 dB"

    def test_gate_no_nan_param_sweep(self, plugin, sine_440):
        """No NaN across diverse parameter combinations."""
        combos = [
            dict(speed_hz=0.1, depth=10.0, feedback=0.0, mix=50.0),
            dict(speed_hz=5.0, depth=100.0, feedback=90.0, mix=100.0),
            dict(speed_hz=0.05, depth=0.0, feedback=-90.0, mix=100.0),
            dict(speed_hz=10.0, depth=50.0, feedback=50.0, mix=25.0),
            dict(speed_hz=1.0, depth=80.0, manual_ms=0.0, mix=100.0),
            dict(speed_hz=1.0, depth=80.0, manual_ms=10.0, mix=100.0),
            dict(speed_hz=2.0, depth=50.0, through_zero=True, mix=100.0),
            dict(speed_hz=2.0, depth=50.0, through_zero=False, mix=100.0),
            dict(speed_hz=1.0, depth=50.0, envelope=100.0, mix=100.0),
            dict(speed_hz=1.0, depth=50.0, stereo=180.0, mix=100.0),
            dict(speed_hz=0.5, depth=50.0, feedback=-95.0, manual_ms=1.0, mix=100.0),
            dict(speed_hz=0.5, depth=50.0, feedback=95.0, manual_ms=10.0, mix=100.0),
        ]
        nan_count = 0
        for params in combos:
            output = process_with_params(plugin, sine_440, **params)
            if has_nan_or_inf(output):
                nan_count += 1
        assert nan_count == 0, f"NaN found in {nan_count}/{len(combos)} combos"

    def test_gate_output_ceiling(self, plugin, sine_440):
        """Peak output should stay below +12 dBFS even with max feedback."""
        output = process_with_params(
            plugin, sine_440,
            speed_hz=2.0, depth=100.0, feedback=95.0, mix=100.0,
        )
        peak = np.max(np.abs(output))
        assert peak < 10.0, f"Peak {peak:.2f}, expected < 10.0"

    def test_gate_silence_floor(self, plugin, silence):
        """Output from silence should be < -80 dB."""
        output = process_with_params(
            plugin, silence,
            speed_hz=1.0, depth=50.0, feedback=50.0, mix=100.0,
        )
        level = rms_db(output)
        assert level < -80, f"Silence floor {level:.1f} dB, expected < -80 dB"

    def test_gate_no_clicks_on_start(self, plugin, sine_440):
        """First 100ms should not have sudden spikes."""
        output = process_with_params(
            plugin, sine_440,
            speed_hz=1.0, depth=50.0, mix=100.0,
        )
        first_100ms = output[:int(SR * 0.1)]
        peak = np.max(np.abs(first_100ms))
        assert peak < 3.0, f"Click in first 100ms: peak {peak:.2f}"

    def test_gate_stereo_balance(self, plugin, sine_440):
        """L/R RMS should be within 1 dB in bypass."""
        output = process_stereo(plugin, sine_440, mix=0.0)
        if output.shape[0] >= 2:
            rms_l = rms_db(output[0])
            rms_r = rms_db(output[1])
            diff = abs(rms_l - rms_r)
            assert diff < 1.0, f"Stereo imbalance: L={rms_l:.1f} R={rms_r:.1f} dB"
