#!/usr/bin/env python3
"""
Telegram Notification & Cooldown Throttling Verification Test Suite
"""

class MockTelegramManager:
    def __init__(self, crit_cooldown_ms=30000, warn_cooldown_ms=120000):
        self.crit_cooldown_ms = crit_cooldown_ms
        self.warn_cooldown_ms = warn_cooldown_ms
        self.last_crit_alert_time = -crit_cooldown_ms
        self.last_warn_alert_time = -warn_cooldown_ms
        self.recovery_pending = False
        self.dispatched_messages = []

    def dispatch(self, state: str, threat: str, temp: float, smoke: int,
                 gps_valid: bool, lat: float, lng: float, current_time_ms: int):
        if state in ("STATE_CRITICAL", "STATE_EMERGENCY"):
            if current_time_ms - self.last_crit_alert_time >= self.crit_cooldown_ms:
                self.last_crit_alert_time = current_time_ms
                self.recovery_pending = True
                maps_link = f"https://maps.google.com/?q={lat:.6f},{lng:.6f}" if gps_valid else "Location Unavailable (Indoor Lab)"
                msg = (f"[CRITICAL ALERT] Fire Detected!\n"
                       f"State: {state} | Threat: {threat}\n"
                       f"Temp: {temp:.1f} C | Smoke: {smoke} ADC\n"
                       f"Maps: {maps_link}")
                self.dispatched_messages.append((current_time_ms, "CRITICAL", msg))
                return True
        elif state == "STATE_WARNING":
            if current_time_ms - self.last_warn_alert_time >= self.warn_cooldown_ms:
                self.last_warn_alert_time = current_time_ms
                self.recovery_pending = True
                msg = f"[WARNING] Elevated Readings! Threat: {threat} | Temp: {temp} | Smoke: {smoke}"
                self.dispatched_messages.append((current_time_ms, "WARNING", msg))
                return True
        elif state == "STATE_NORMAL":
            if self.recovery_pending:
                self.recovery_pending = False
                msg = "[RECOVERY] System returned to normal conditions."
                self.dispatched_messages.append((current_time_ms, "RECOVERY", msg))
                return True
        return False

def run_tests():
    print("[TEST SUITE] Testing Telegram Alert Generation & Cooldown Throttling...")
    mgr = MockTelegramManager()

    # 1. First Critical Fire Alert should fire immediately
    fired = mgr.dispatch("STATE_CRITICAL", "OPTICAL_FLAME_DETECTED", 52.3, 2400,
                         True, 13.0827, 80.2707, current_time_ms=1000)
    assert fired, "Initial critical alert failed to fire!"
    assert len(mgr.dispatched_messages) == 1
    assert "https://maps.google.com/?q=13.082700,80.270700" in mgr.dispatched_messages[0][2]
    print("  [PASS] Test 1: Immediate critical fire alert dispatched with valid Google Maps URL")

    # 2. Repeated trigger within 10 seconds must be throttled by 30-second cooldown
    fired = mgr.dispatch("STATE_CRITICAL", "OPTICAL_FLAME_DETECTED", 54.0, 2500,
                         True, 13.0827, 80.2707, current_time_ms=11000)
    assert not fired, "Cooldown failed to suppress repeated alert spam!"
    assert len(mgr.dispatched_messages) == 1
    print("  [PASS] Test 2: Rapid alert spam suppressed by 30-second cooldown")

    # 3. Trigger after cooldown elapsed (at t = 32000 ms) should fire
    fired = mgr.dispatch("STATE_CRITICAL", "OPTICAL_FLAME_DETECTED", 55.1, 2550,
                         True, 13.0827, 80.2707, current_time_ms=32000)
    assert fired, "Alert after cooldown elapsed failed to fire!"
    assert len(mgr.dispatched_messages) == 2
    print("  [PASS] Test 3: Alert allowed after cooldown duration elapsed")

    # 4. System returns to NORMAL -> triggers single RECOVERY alert
    fired = mgr.dispatch("STATE_NORMAL", "THREAT_NONE", 26.0, 600,
                         True, 13.0827, 80.2707, current_time_ms=45000)
    assert fired, "Recovery alert failed to trigger!"
    assert mgr.dispatched_messages[-1][1] == "RECOVERY"
    print("  [PASS] Test 4: Single recovery notification triggered on transition to NORMAL")

    # 5. Subsequent NORMAL checks do NOT re-send recovery alert
    fired = mgr.dispatch("STATE_NORMAL", "THREAT_NONE", 26.0, 600,
                         True, 13.0827, 80.2707, current_time_ms=46000)
    assert not fired, "Duplicate recovery message sent!"
    print("  [PASS] Test 5: Duplicate recovery messages prevented")

    print("[TEST SUITE] All Telegram Notification Logic Tests Passed Successfully!\n")

if __name__ == "__main__":
    run_tests()

