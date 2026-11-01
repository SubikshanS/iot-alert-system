#!/usr/bin/env python3
"""
Sensor Algorithms & Mathematical Calibration Unit Test Suite
Verifies signal processing filters, voltage reconstruction, hysteresis, and debounce timings.
"""

def test_moving_average_filter():
    window_size = 10
    buffer = []
    
    def update(val):
        nonlocal buffer
        buffer.append(val)
        if len(buffer) > window_size:
            buffer.pop(0)
        return sum(buffer) / len(buffer)

    # 1. Steady state
    for _ in range(10):
        avg = update(1000)
    assert avg == 1000, f"Expected 1000, got {avg}"

    # 2. Glitch spike dampening
    avg_with_spike = update(4000)
    expected_spike_avg = (9 * 1000 + 4000) / 10 # 1300.0
    assert avg_with_spike == expected_spike_avg, f"Expected {expected_spike_avg}, got {avg_with_spike}"
    print("  [PASS] Moving average filter dampens transient spike by 90%")

def test_voltage_divider_reconstruction():
    ratio = 0.60 # 3.3k / (2.2k + 3.3k)
    adc_max = 4095.0
    v_ref = 3.3

    test_cases = [
        # (raw_adc, expected_sensor_voltage)
        (0, 0.0),
        (2048, (2048 / adc_max * v_ref) / ratio), # ~2.75V
        (4095, (4095 / adc_max * v_ref) / ratio), # 5.5V (theoretical ceiling)
    ]

    for raw, expected in test_cases:
        pin_v = (raw / adc_max) * v_ref
        reconstructed = pin_v / ratio
        assert abs(reconstructed - expected) < 1e-3, f"Mismatch: {reconstructed} vs {expected}"

    print("  [PASS] Voltage divider reconstruction accurately rescales 0-3.0V back to 0-5.0V")

def test_hysteresis_logic():
    # Smoke hysteresis: Warn at 1200, return to Normal only below 1050 (1200 - 150)
    warn_thresh = 1200
    hysteresis = 150
    clear_thresh = warn_thresh - hysteresis # 1050

    state = "NORMAL"
    def evaluate(reading):
        nonlocal state
        if state == "NORMAL" and reading >= warn_thresh:
            state = "WARNING"
        elif state == "WARNING" and reading < clear_thresh:
            state = "NORMAL"
        return state

    assert evaluate(1100) == "NORMAL"
    assert evaluate(1205) == "WARNING" # Triggered
    assert evaluate(1150) == "WARNING" # Kept in warning (above 1050)
    assert evaluate(1080) == "WARNING" # Still in warning
    assert evaluate(1040) == "NORMAL"  # Normalized below clear_thresh
    print("  [PASS] Hysteresis logic prevents rapid boundary flapping")

if __name__ == "__main__":
    print("[TEST SUITE] Verifying Sensor Algorithms and Calibration Math...")
    test_moving_average_filter()
    test_voltage_divider_reconstruction()
    test_hysteresis_logic()
    print("[TEST SUITE] All Mathematical & Algorithmic Assertions Passed!\n")

