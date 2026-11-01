#!/usr/bin/env python3
"""
Emergency Button Debounce/Latch & Alarm Controller Test Suite
"""

class MockEmergencyController:
    def __init__(self, debounce_ms=50, reset_hold_ms=3000):
        self.debounce_ms = debounce_ms
        self.reset_hold_ms = reset_hold_ms
        self.is_latched = False
        self.debounced_state = False
        self.last_physical_state = False
        self.last_debounce_time = 0
        self.press_start_time = 0
        self.long_press_ack = False

    def poll(self, pin_is_low, current_time_ms):
        # Debounce check
        if pin_is_low != self.last_physical_state:
            self.last_debounce_time = current_time_ms

        if (current_time_ms - self.last_debounce_time) > self.debounce_ms:
            if pin_is_low != self.debounced_state:
                self.debounced_state = pin_is_low
                if self.debounced_state:
                    self.press_start_time = current_time_ms
                    self.long_press_ack = False
                    if not self.is_latched:
                        self.is_latched = True

        # Long press reset check
        if self.debounced_state and self.is_latched:
            if (current_time_ms - self.press_start_time >= self.reset_hold_ms) and not self.long_press_ack:
                self.long_press_ack = True
                self.is_latched = False

        self.last_physical_state = pin_is_low
        return self.is_latched

def run_tests():
    print("[TEST SUITE] Testing Emergency Button Debounce, Latching & Reset...")
    controller = MockEmergencyController()

    # 1. Noise glitch (< 50ms) should NOT trigger latch
    controller.poll(pin_is_low=True, current_time_ms=0)
    controller.poll(pin_is_low=True, current_time_ms=20)
    controller.poll(pin_is_low=False, current_time_ms=30)
    assert not controller.is_latched, "Noise glitch triggered latch!"
    print("  [PASS] Test 1: Electrical noise glitch (30ms pulse) successfully filtered")

    # 2. Genuine Press (> 50ms) triggers latch
    controller.poll(pin_is_low=True, current_time_ms=100)
    controller.poll(pin_is_low=True, current_time_ms=160)
    assert controller.is_latched, "Genuine press failed to latch!"
    print("  [PASS] Test 2: Genuine press latches emergency state")

    # 3. Button Release does NOT clear latch (industrial latching behavior)
    controller.poll(pin_is_low=False, current_time_ms=300)
    controller.poll(pin_is_low=False, current_time_ms=400)
    assert controller.is_latched, "Emergency latch released prematurely!"
    print("  [PASS] Test 3: Emergency state remains locked after physical button release")

    # 4. Long press (>= 3000ms hold) resets the emergency
    controller.poll(pin_is_low=True, current_time_ms=1000)
    controller.poll(pin_is_low=True, current_time_ms=1060) # Pressed
    controller.poll(pin_is_low=True, current_time_ms=4100) # Held for 3100ms
    assert not controller.is_latched, "Long press hold failed to clear latch!"
    print("  [PASS] Test 4: 3-second long press successfully disarms/resets the emergency")

    print("[TEST SUITE] All Emergency Controller Tests Passed Successfully!\n")

if __name__ == "__main__":
    run_tests()

