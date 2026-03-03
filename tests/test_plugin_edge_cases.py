"""
Edge cases and stress tests — extreme params, sample rates, block sizes.
"""

import numpy as np
import pytest
from conftest import SR, process_with_params, has_nan_or_inf, rms_db


class TestExtremeFeedback:
    def test_max_positive_feedback(self, plugin, sine_440):
        """Maximum positive feedback should not cause runaway."""
        output = process_with_params(
            plugin, sine_440,
            feedback=95.0, depth=100.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)
        peak = np.max(np.abs(output))
        assert peak < 10.0, f"Runaway: peak {peak:.2f}"

    def test_max_negative_feedback(self, plugin, sine_440):
        """Maximum negative feedback should not cause runaway."""
        output = process_with_params(
            plugin, sine_440,
            feedback=-95.0, depth=100.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)
        peak = np.max(np.abs(output))
        assert peak < 10.0, f"Runaway: peak {peak:.2f}"


class TestExtremeDepth:
    def test_max_depth(self, plugin, sine_440):
        output = process_with_params(
            plugin, sine_440, depth=100.0, speed_hz=5.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_zero_depth(self, plugin, sine_440):
        output = process_with_params(
            plugin, sine_440, depth=0.0, speed_hz=5.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)


class TestExtremeManual:
    def test_max_manual_delay(self, plugin, sine_440):
        output = process_with_params(
            plugin, sine_440, manual_ms=10.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_zero_manual_delay(self, plugin, sine_440):
        output = process_with_params(
            plugin, sine_440, manual_ms=0.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)


class TestAllParamsExtreme:
    def test_all_max(self, plugin, sine_440):
        """Every parameter at maximum should not crash."""
        output = process_with_params(
            plugin, sine_440,
            speed_hz=10.0, depth=100.0, manual_ms=10.0,
            feedback=95.0, stereo=180.0,
            through_zero=True, envelope=100.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_all_min(self, plugin, sine_440):
        """Every parameter at minimum should not crash."""
        output = process_with_params(
            plugin, sine_440,
            speed_hz=0.05, depth=0.0, manual_ms=0.0,
            feedback=-95.0, stereo=0.0,
            through_zero=False, envelope=0.0, mix=0.0,
        )
        assert not has_nan_or_inf(output)


@pytest.mark.parametrize("sr", [22050, 44100, 48000, 88200, 96000])
class TestSampleRates:
    def test_no_nan_at_sample_rate(self, plugin, sr):
        t = np.arange(int(sr * 0.5)) / sr
        sine = np.sin(2 * np.pi * 440 * t).astype(np.float32)
        output = process_with_params(
            plugin, sine, sr=sr,
            speed_hz=1.0, depth=50.0, mix=100.0,
        )
        assert not has_nan_or_inf(output)

    def test_produces_output_at_sample_rate(self, plugin, sr):
        t = np.arange(int(sr * 0.5)) / sr
        sine = np.sin(2 * np.pi * 440 * t).astype(np.float32)
        output = process_with_params(
            plugin, sine, sr=sr,
            speed_hz=1.0, depth=50.0, mix=100.0,
        )
        assert rms_db(output) > -60


@pytest.mark.parametrize("block_size", [64, 128, 256, 512, 1024])
class TestBlockSizes:
    def test_no_nan_at_block_size(self, plugin, block_size):
        """Process in specific block sizes."""
        n_samples = SR
        sine = np.sin(2 * np.pi * 440 * np.arange(n_samples) / SR).astype(np.float32)
        audio = sine[np.newaxis, :]

        plugin.reset()
        plugin.speed_hz = 1.0
        plugin.depth = 50.0
        plugin.mix = 100.0

        chunks = []
        for start in range(0, n_samples, block_size):
            end = min(start + block_size, n_samples)
            chunk = audio[:, start:end]
            out = plugin.process(chunk, sample_rate=float(SR))
            chunks.append(out)

        output = np.concatenate(chunks, axis=1)
        assert not has_nan_or_inf(output)

    def test_produces_output_at_block_size(self, plugin, block_size):
        n_samples = SR
        sine = np.sin(2 * np.pi * 440 * np.arange(n_samples) / SR).astype(np.float32)
        audio = sine[np.newaxis, :]

        plugin.reset()
        plugin.speed_hz = 1.0
        plugin.depth = 50.0
        plugin.mix = 100.0

        chunks = []
        for start in range(0, n_samples, block_size):
            end = min(start + block_size, n_samples)
            chunk = audio[:, start:end]
            out = plugin.process(chunk, sample_rate=float(SR))
            chunks.append(out)

        output = np.concatenate(chunks, axis=1)
        assert rms_db(output[0]) > -60
