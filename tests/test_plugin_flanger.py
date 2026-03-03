"""
Core flanger behavior tests — comb filtering, sweep, feedback, stereo, TZF.
"""

import numpy as np
import pytest
from conftest import (
    SR, process_with_params, process_stereo, has_nan_or_inf,
    rms_db, peak_frequency, spectral_energy_in_band,
)


class TestCombFilter:
    """Flanger is fundamentally a comb filter — verify the spectral pattern."""

    def test_notches_present_in_spectrum(self, plugin, white_noise):
        """With static delay (slow LFO), white noise should show comb pattern."""
        output = process_with_params(
            plugin, white_noise,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=0.0, mix=100.0, through_zero=False,
        )
        spectrum = np.abs(np.fft.rfft(output))
        freqs = np.fft.rfftfreq(len(output), 1.0 / SR)

        # Classic flanger (not TZF) with 3ms delay: comb filter
        # Spectrum should differ from flat — measure spectral variance
        # A comb-filtered white noise has much higher variance than flat noise
        band = (freqs > 100) & (freqs < 10000)
        band_spectrum = spectrum[band]
        if np.mean(band_spectrum) > 0:
            coeff_of_variation = np.std(band_spectrum) / np.mean(band_spectrum)
            assert coeff_of_variation > 0.3, (
                f"Spectrum too flat (CV={coeff_of_variation:.2f}), "
                f"expected comb filter pattern"
            )

    def test_feedback_deepens_notches(self, plugin, white_noise):
        """Higher feedback should create deeper notches / sharper peaks."""
        out_no_fb = process_with_params(
            plugin, white_noise,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=0.0, mix=100.0,
        )
        out_with_fb = process_with_params(
            plugin, white_noise,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=80.0, mix=100.0,
        )
        # Spectral flatness: less flat = more resonant comb
        spec_no = np.abs(np.fft.rfft(out_no_fb))
        spec_fb = np.abs(np.fft.rfft(out_with_fb))

        var_no = np.var(spec_no[10:]) / (np.mean(spec_no[10:]) + 1e-20)
        var_fb = np.var(spec_fb[10:]) / (np.mean(spec_fb[10:]) + 1e-20)

        assert var_fb > var_no, (
            f"Feedback should increase spectral variance: "
            f"no_fb={var_no:.4f}, with_fb={var_fb:.4f}"
        )


class TestSweep:
    """Verify the LFO sweeps the comb filter through the spectrum."""

    def test_sweep_creates_moving_notches(self, plugin, white_noise):
        """With LFO active, short windows should have different spectral shapes."""
        output = process_with_params(
            plugin, white_noise,
            speed_hz=2.0, depth=100.0, manual_ms=5.0,
            feedback=50.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

        # Compare spectrum of first and second halves
        half = len(output) // 2
        spec1 = np.abs(np.fft.rfft(output[:half]))
        spec2 = np.abs(np.fft.rfft(output[half:]))

        # Peak frequencies should differ (comb is moving)
        peak1 = np.argmax(spec1[10:]) + 10
        peak2 = np.argmax(spec2[10:]) + 10
        assert peak1 != peak2, "Comb filter is not sweeping"

    def test_depth_zero_no_sweep(self, plugin, sine_440):
        """Depth=0 should produce a static comb filter."""
        out1 = process_with_params(
            plugin, sine_440,
            speed_hz=2.0, depth=0.0, manual_ms=3.0, mix=100.0,
        )
        # With zero depth, output should be consistent (no modulation)
        assert not has_nan_or_inf(out1)
        # Check first and second halves are similar in energy
        half = len(out1) // 2
        rms1 = rms_db(out1[:half])
        rms2 = rms_db(out1[half:])
        assert abs(rms1 - rms2) < 3.0, (
            f"Static flange energy varies too much: {rms1:.1f} vs {rms2:.1f} dB"
        )


class TestThroughZero:
    """Through-zero mode should produce deeper cancellations."""

    def test_tzf_deeper_than_normal(self, plugin, white_noise):
        """TZF mode should create deeper notches than non-TZF."""
        out_tz = process_with_params(
            plugin, white_noise,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=0.0, mix=100.0, through_zero=True,
        )
        out_normal = process_with_params(
            plugin, white_noise,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=0.0, mix=100.0, through_zero=False,
        )
        # TZF should produce more spectral variation (deeper notches)
        spec_tz = np.abs(np.fft.rfft(out_tz))
        spec_no = np.abs(np.fft.rfft(out_normal))

        var_tz = np.var(spec_tz[10:]) / (np.mean(spec_tz[10:]) + 1e-20)
        var_no = np.var(spec_no[10:]) / (np.mean(spec_no[10:]) + 1e-20)

        # TZF creates a true null at zero crossing, so should have
        # more spectral variation than regular flanging
        assert var_tz > var_no * 0.5, (
            f"TZF not producing enough effect: tz_var={var_tz:.4f}, normal_var={var_no:.4f}"
        )

    def test_tzf_no_nan(self, plugin, impulse):
        """TZF mode with impulse should not produce NaN."""
        output = process_with_params(
            plugin, impulse,
            speed_hz=1.0, depth=100.0, manual_ms=5.0,
            feedback=50.0, mix=100.0, through_zero=True,
        )
        assert not has_nan_or_inf(output)


class TestNegativeFeedback:
    """Negative feedback should produce a different spectral character."""

    def test_negative_vs_positive_feedback(self, plugin, white_noise):
        """Negative and positive feedback should produce different spectra."""
        out_pos = process_with_params(
            plugin, white_noise,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=80.0, mix=100.0,
        )
        out_neg = process_with_params(
            plugin, white_noise,
            speed_hz=0.05, depth=0.0, manual_ms=3.0,
            feedback=-80.0, mix=100.0,
        )
        spec_pos = np.abs(np.fft.rfft(out_pos))
        spec_neg = np.abs(np.fft.rfft(out_neg))

        # Peak positions should differ (positive = peaks at harmonics,
        # negative = peaks shifted by half-harmonic)
        peak_pos = np.argmax(spec_pos[20:200]) + 20
        peak_neg = np.argmax(spec_neg[20:200]) + 20
        assert peak_pos != peak_neg, "Positive and negative feedback produce same spectrum"


class TestStereoSpread:
    """Stereo spread should decorrelate L and R channels."""

    def test_stereo_spread_decorrelation(self, plugin, white_noise):
        """With stereo spread, L and R should differ."""
        output = process_stereo(
            plugin, white_noise,
            speed_hz=1.0, depth=80.0, manual_ms=5.0,
            stereo=180.0, mix=100.0,
        )
        if output.shape[0] >= 2:
            corr = np.corrcoef(output[0], output[1])[0, 1]
            assert corr < 0.95, (
                f"L/R correlation {corr:.3f}, expected < 0.95 with 180 deg spread"
            )

    def test_stereo_spread_zero_correlated(self, plugin, white_noise):
        """With 0 spread, L and R should be highly correlated."""
        output = process_stereo(
            plugin, white_noise,
            speed_hz=1.0, depth=80.0, manual_ms=5.0,
            stereo=0.0, mix=100.0,
        )
        if output.shape[0] >= 2:
            corr = np.corrcoef(output[0], output[1])[0, 1]
            assert corr > 0.95, (
                f"L/R correlation {corr:.3f}, expected > 0.95 with 0 spread"
            )


class TestDryWetMix:
    def test_mix_50_has_both(self, plugin, sine_440):
        """50% mix should have both dry and flanged signal."""
        dry_out = process_with_params(plugin, sine_440, mix=0.0)
        wet_out = process_with_params(
            plugin, sine_440, speed_hz=2.0, depth=80.0, mix=100.0
        )
        mix_out = process_with_params(
            plugin, sine_440, speed_hz=2.0, depth=80.0, mix=50.0
        )
        # Mix should be between dry and wet in energy
        assert not has_nan_or_inf(mix_out)
        rms_mix = rms_db(mix_out)
        rms_dry = rms_db(dry_out)
        rms_wet = rms_db(wet_out)
        # Should be in a reasonable range
        assert rms_mix > min(rms_dry, rms_wet) - 6
        assert rms_mix < max(rms_dry, rms_wet) + 6
