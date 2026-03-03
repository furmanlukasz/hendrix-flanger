"""
Tier 4: Quality gates — release criteria that must pass before shipping.
"""

import numpy as np
import pytest
from conftest import SR, has_nan_or_inf, process_with_params, rms_db


class TestQualityGates:
    """Release-blocking quality checks."""

    def test_gate_bypass_snr(self, plugin):
        """Bypass path must have > 80 dB SNR."""
        t = np.arange(SR) / SR
        sig = np.sin(2 * np.pi * 1000 * t).astype(np.float32)
        output = process_with_params(plugin, sig, bypass=True)
        noise = output - sig[: len(output)]
        snr = rms_db(sig) - rms_db(noise)
        assert snr > 80, f"Bypass SNR only {snr:.1f} dB"

    def test_gate_no_nan_param_sweep(self, plugin):
        """Sweep through 20 parameter combos without NaN."""
        t = np.arange(SR) / SR
        sig = np.sin(2 * np.pi * 440 * t).astype(np.float32)
        for gain in [-60, -30, -6, 0, 6, 12]:
            for dw in [0, 50, 100]:
                out = process_with_params(
                    plugin, sig, gain_db=float(gain), dry_wet=float(dw)
                )
                assert not has_nan_or_inf(out), f"NaN at gain={gain}, dw={dw}"

    def test_gate_output_ceiling(self, plugin):
        """Output should never exceed +-10.0 (prevent speaker damage)."""
        t = np.arange(SR) / SR
        sig = np.sin(2 * np.pi * 440 * t).astype(np.float32)
        output = process_with_params(plugin, sig, gain_db=12.0, dry_wet=100.0)
        peak = np.max(np.abs(output))
        assert peak < 10.0, f"Output peak {peak:.2f}"

    def test_gate_silence_floor(self, plugin):
        """Silence input must produce < -80 dB output."""
        sig = np.zeros(SR, dtype=np.float32)
        output = process_with_params(plugin, sig, gain_db=0.0, dry_wet=100.0)
        level = rms_db(output)
        assert level < -80, f"Silence floor at {level:.1f} dB"

    def test_gate_no_clicks_on_start(self, plugin):
        """First 256 samples shouldn't have a loud click."""
        sig = np.zeros(SR, dtype=np.float32)
        output = process_with_params(plugin, sig, gain_db=0.0, dry_wet=100.0)
        first_chunk_peak = np.max(np.abs(output[:256]))
        assert first_chunk_peak < 0.01, f"Startup click: {first_chunk_peak:.4f}"

    def test_gate_stereo_balance(self, plugin):
        """Stereo signal should remain balanced."""
        t = np.arange(SR) / SR
        mono = np.sin(2 * np.pi * 440 * t).astype(np.float32)
        stereo = np.stack([mono, mono])  # identical L/R

        plugin.reset()
        plugin.gain_db = 0.0
        plugin.dry_wet = 100.0
        output = plugin.process(stereo, sample_rate=float(SR))

        if output.shape[0] >= 2:
            diff = np.max(np.abs(output[0] - output[1]))
            assert diff < 0.001, f"Stereo imbalance: {diff:.6f}"
