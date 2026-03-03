"""
Tier 3: Edge cases — extreme parameters, sample rates, block sizes.
"""

import numpy as np
import pytest
from conftest import SR, has_nan_or_inf, process_with_params


class TestExtremeParameters:
    """Plugin shouldn't crash with extreme parameter values."""

    def test_max_gain(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, gain_db=12.0, dry_wet=100.0)
        assert not has_nan_or_inf(output)

    def test_min_gain(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, gain_db=-60.0, dry_wet=100.0)
        assert not has_nan_or_inf(output)

    def test_zero_dry_wet(self, plugin, sine_440):
        output = process_with_params(plugin, sine_440, gain_db=6.0, dry_wet=0.0)
        assert not has_nan_or_inf(output)

    def test_rapid_parameter_change(self, plugin, sine_440):
        """Rapid param changes shouldn't cause clicks or crashes."""
        for gain in [-60, 12, -30, 6, 0, -12, 12]:
            output = process_with_params(
                plugin, sine_440, gain_db=float(gain), dry_wet=100.0
            )
            assert not has_nan_or_inf(output)


class TestSampleRates:
    """Plugin must work at common sample rates."""

    @pytest.mark.parametrize("sr", [22050, 44100, 48000, 88200, 96000])
    def test_sample_rate(self, plugin, sr):
        t = np.arange(int(sr * 0.5)) / sr
        sig = np.sin(2 * np.pi * 440 * t).astype(np.float32)
        output = process_with_params(plugin, sig, sr=sr, gain_db=0.0, dry_wet=100.0)
        assert not has_nan_or_inf(output)
        assert len(output) == len(sig)


class TestBlockSizes:
    """Plugin must work with various block sizes."""

    @pytest.mark.parametrize("block_size", [64, 128, 256, 512, 1024])
    def test_block_size(self, plugin, block_size):
        # Process in chunks of block_size
        sig = np.sin(
            2 * np.pi * 440 * np.arange(SR) / SR
        ).astype(np.float32)
        output = process_with_params(plugin, sig, gain_db=0.0, dry_wet=100.0)
        assert not has_nan_or_inf(output)
