#!/usr/bin/env python3
"""
Multi-Sensor Detection & State Machine Rule Engine Verification Suite
Tests the complete threat priority hierarchy, dual-threat correlation, and hysteresis.
"""

class DetectionEngine:
    def __init__(self, smoke_warn=1200, smoke_crit=2200, temp_warn=45.0, temp_crit=55.0,
                 smoke_hyst=150, temp_hyst=3.0):
        self.smoke_warn = smoke_warn
        self.smoke_crit = smoke_crit
        self.temp_warn = temp_warn
        self.temp_crit = temp_crit
        self.smoke_hyst = smoke_hyst
        self.temp_hyst = temp_hyst
        self.current_state = "STATE_NORMAL"
        self.active_threat = "THREAT_NONE"

    def evaluate(self, emergency_pressed, flame_detected, smoke_raw, temp,
                 dht_valid=True, mq2_valid=True):
        # Priority 1: Emergency Button
        if emergency_pressed:
            self.active_threat = "THREAT_MANUAL_EMERGENCY"
            self.current_state = "STATE_EMERGENCY"
            return self.current_state, self.active_threat

        # Priority 2: Optical Flame
        if flame_detected:
            self.active_threat = "THREAT_FLAME_DETECTED"
            self.current_state = "STATE_CRITICAL"
            return self.current_state, self.active_threat

        # Priority 3: Severe Smoke or Dual Elevated
        severe_smoke = (smoke_raw >= self.smoke_crit)
        dual_elevated = (smoke_raw >= self.smoke_warn and temp >= self.temp_warn)
        if severe_smoke or dual_elevated:
            self.active_threat = "THREAT_DUAL_CRITICAL"
            self.current_state = "STATE_CRITICAL"
            return self.current_state, self.active_threat

        # Critical hysteresis
        if self.current_state == "STATE_CRITICAL":
            smoke_crit_clear = max(0, self.smoke_crit - self.smoke_hyst)
            temp_crit_clear = max(0.0, self.temp_crit - self.temp_hyst)
            if smoke_raw >= smoke_crit_clear or temp >= temp_crit_clear:
                self.active_threat = "THREAT_DUAL_CRITICAL"
                return self.current_state, self.active_threat

        # Priority 4: Warning State with Hysteresis
        smoke_warn_clear = max(0, self.smoke_warn - self.smoke_hyst)
        temp_warn_clear = max(0.0, self.temp_warn - self.temp_hyst)

        if self.current_state == "STATE_WARNING":
            if smoke_raw >= smoke_warn_clear:
                self.active_threat = "THREAT_SMOKE_WARN"
                return self.current_state, self.active_threat
            if temp >= temp_warn_clear:
                self.active_threat = "THREAT_TEMP_WARN"
                return self.current_state, self.active_threat
        else:
            if smoke_raw >= self.smoke_warn:
                self.active_threat = "THREAT_SMOKE_WARN"
                self.current_state = "STATE_WARNING"
                return self.current_state, self.active_threat
            if temp >= self.temp_warn:
                self.active_threat = "THREAT_TEMP_WARN"
                self.current_state = "STATE_WARNING"
                return self.current_state, self.active_threat

        # Priority 5: Sensor Fault
        if not dht_valid or not mq2_valid:
            self.active_threat = "THREAT_SENSOR_FAULT"
            self.current_state = "STATE_SENSOR_ERROR"
            return self.current_state, self.active_threat

        # Priority 6: Normal
        self.active_threat = "THREAT_NONE"
        self.current_state = "STATE_NORMAL"
        return self.current_state, self.active_threat

def run_tests():
    print("[TEST SUITE] Executing Multi-Sensor Detection Rule Tests...")
    engine = DetectionEngine()

    # 1. Test Baseline Normal State
    state, threat = engine.evaluate(False, False, 550, 26.5)
    assert state == "STATE_NORMAL" and threat == "THREAT_NONE"
    print("  [PASS] Test 1: Clean baseline resolves to STATE_NORMAL")

    # 2. Test Elevated Smoke Alone (Warning)
    state, threat = engine.evaluate(False, False, 1250, 26.5)
    assert state == "STATE_WARNING" and threat == "THREAT_SMOKE_WARN"
    print("  [PASS] Test 2: Elevated smoke alone resolves to STATE_WARNING")

    # 3. Test Smoke Hysteresis in Warning State
    state, threat = engine.evaluate(False, False, 1100, 26.5) # Above 1050 clear boundary
    assert state == "STATE_WARNING"
    state, threat = engine.evaluate(False, False, 1020, 26.5) # Below 1050 clear boundary
    assert state == "STATE_NORMAL"
    print("  [PASS] Test 3: Smoke warning hysteresis prevents flapping")

    # 4. Test Dual-Threat (Elevated Smoke + Elevated Temperature)
    state, threat = engine.evaluate(False, False, 1300, 48.0)
    assert state == "STATE_CRITICAL" and threat == "THREAT_DUAL_CRITICAL"
    print("  [PASS] Test 4: Combined smoke + temperature escalates to STATE_CRITICAL")

    # 5. Test Optical Flame Override (Life-Safety Axiom)
    # Even if smoke and temp are very low/clean!
    engine = DetectionEngine()
    state, threat = engine.evaluate(False, True, 400, 20.0)
    assert state == "STATE_CRITICAL" and threat == "THREAT_FLAME_DETECTED"
    print("  [PASS] Test 5: Optical flame immediately overrides all baseline readings")

    # 6. Test Manual Emergency Button Override (Highest Priority)
    state, threat = engine.evaluate(True, False, 500, 25.0)
    assert state == "STATE_EMERGENCY" and threat == "THREAT_MANUAL_EMERGENCY"
    print("  [PASS] Test 6: Emergency button press overrides all states")

    # 7. Test Sensor Fault Isolation
    engine = DetectionEngine()
    state, threat = engine.evaluate(False, False, 500, 25.0, dht_valid=False)
    assert state == "STATE_SENSOR_ERROR" and threat == "THREAT_SENSOR_FAULT"
    print("  [PASS] Test 7: Disconnected DHT11 transitions to STATE_SENSOR_ERROR without crash")

    print("[TEST SUITE] All Multi-Sensor Detection Rule Tests Passed Successfully!\n")

if __name__ == "__main__":
    run_tests()

