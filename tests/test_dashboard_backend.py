#!/usr/bin/env python3
"""
Dashboard Backend In-Memory Handler & Logic Verification Test Suite
"""

import sys
import os
import json
import io

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'dashboard'))
import server

def run_tests():
    print("[TEST SUITE] Verifying Dashboard Backend Data Handlers in Memory...")

    # 1. Test Static Files Exist and Have Expected Content
    index_path = os.path.join(server.DASHBOARD_DIR, 'index.html')
    style_path = os.path.join(server.DASHBOARD_DIR, 'css', 'style.css')
    app_path = os.path.join(server.DASHBOARD_DIR, 'js', 'app.js')

    assert os.path.exists(index_path), "index.html is missing"
    assert os.path.exists(style_path), "style.css is missing"
    assert os.path.exists(app_path), "app.js is missing"

    with open(index_path, 'r', encoding='utf-8') as f:
        html = f.read()
        assert "1. System Overview" in html
        assert "2. CURRENT ALERT STATUS BANNER" in html or "alertBanner" in html
        assert "3. Temperature" in html
        assert "4. Smoke" in html
        assert "5. Optical Flame" in html
        assert "6. Geolocation" in html
        assert "7. Network Connectivity" in html
        assert "8. Sensor Health" in html
        assert "9. Historical Sensor Telemetry" in html
        assert "10. Emergency Event Log" in html
        print("  [PASS] Test 1: All 10 required dashboard sections verified in index.html")

    # 2. Test JSON Schema in server.py
    state = server.sim_state
    required_top_keys = ["system", "sensors", "health", "gps", "thresholds", "event_log"]
    for k in required_top_keys:
        assert k in state, f"Missing key in sim_state: {k}"
    print("  [PASS] Test 2: Telemetry JSON schema contains all required subsystems")

    # 3. Test Simulation State Transitions
    class DummyHandler:
        def __init__(self):
            pass
        trigger_simulation = server.DashboardHandler.trigger_simulation

    dummy = DummyHandler()

    # Trigger flame
    dummy.trigger_simulation("flame")
    assert server.sim_state["sensors"]["flame_detected"] == True
    assert server.sim_state["system"]["state"] == "CRITICAL"
    assert server.sim_state["system"]["threat"] == "OPTICAL_FLAME_DETECTED"
    print("  [PASS] Test 3: Simulation trigger 'flame' asserts CRITICAL state and threat")

    # Trigger smoke
    dummy.trigger_simulation("smoke")
    assert server.sim_state["sensors"]["smoke_raw"] >= 1200
    assert server.sim_state["system"]["state"] == "WARNING"
    print("  [PASS] Test 4: Simulation trigger 'smoke' asserts WARNING state and elevated ADC")

    # Trigger button
    dummy.trigger_simulation("button")
    assert server.sim_state["sensors"]["emergency_pressed"] == True
    assert server.sim_state["system"]["state"] == "EMERGENCY"
    print("  [PASS] Test 5: Simulation trigger 'button' asserts EMERGENCY state")

    # Restore normal
    dummy.trigger_simulation("clear")
    assert server.sim_state["sensors"]["flame_detected"] == False
    assert server.sim_state["sensors"]["emergency_pressed"] == False
    assert server.sim_state["system"]["state"] == "NORMAL"
    assert server.sim_state["system"]["threat"] == "NONE"
    print("  [PASS] Test 6: Simulation restore 'clear' cleanly resets to NORMAL baseline")

    print("[TEST SUITE] All Dashboard Frontend & Backend Tests Passed Successfully!\n")

if __name__ == "__main__":
    run_tests()
