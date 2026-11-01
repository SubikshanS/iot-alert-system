#!/usr/bin/env python3
"""
Lightweight Communication Backend & Web Dashboard Server
Serves static dashboard files, relays live ESP32 REST API telemetry, and provides
realistic hardware simulation when testing without physical hardware.
"""

import sys
import os
import json
import time
import math
import random
from http.server import HTTPServer, SimpleHTTPRequestHandler
import urllib.request
import urllib.error

PORT = 8000
DASHBOARD_DIR = os.path.dirname(os.path.abspath(__file__))

# Global Simulated State (for demonstration without physical hardware)
sim_state = {
    "system": {
        "state": "NORMAL",
        "previous_state": "BOOT",
        "threat": "NONE",
        "uptime_seconds": 0,
        "free_heap": 294820,
        "reset_reason": "Power-on reset",
        "wifi_connected": True,
        "ip_address": "192.168.1.105",
        "rssi": -58
    },
    "sensors": {
        "temperature": 27.2,
        "humidity": 54.0,
        "smoke_raw": 580,
        "smoke_voltage": 0.78,
        "flame_detected": False,
        "emergency_pressed": False
    },
    "health": {
        "dht_valid": True,
        "mq2_valid": True,
        "flame_valid": True,
        "gps_valid": True
    },
    "gps": {
        "has_fix": True,
        "latitude": 13.082700,
        "longitude": 80.270700,
        "satellites": 8,
        "altitude_m": 15.2,
        "hdop": 0.9,
        "timestamp": "2026-11-01 19:45:00 UTC",
        "maps_url": "https://maps.google.com/?q=13.082700,80.270700"
    },
    "thresholds": {
        "smoke_warn": 1200,
        "smoke_crit": 2200,
        "temp_warn": 45.0,
        "temp_crit": 55.0
    },
    "event_log": [
        {"timestamp": "19:40:02", "type": "INFO", "message": "System booted and telemetry initialized"},
        {"timestamp": "19:40:05", "type": "INFO", "message": "Wi-Fi connected to Station AP (IP: 192.168.1.105)"},
        {"timestamp": "19:40:12", "type": "INFO", "message": "GPS 3D satellite fix acquired (8 satellites)"}
    ]
}

start_time = time.time()

class DashboardHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DASHBOARD_DIR, **kwargs)

    def end_headers(self):
        # Enable universal CORS headers
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.end_headers()

    def do_GET(self):
        if self.path == '/api/status':
            self.handle_get_status()
        elif self.path.startswith('/api/relay?ip='):
            self.handle_relay_esp32()
        else:
            super().do_GET()

    def do_POST(self):
        if self.path == '/api/emergency/reset':
            sim_state["sensors"]["emergency_pressed"] = False
            sim_state["system"]["state"] = "NORMAL"
            sim_state["system"]["threat"] = "NONE"
            sim_state["event_log"].append({
                "timestamp": time.strftime("%H:%M:%S"),
                "type": "NORMAL",
                "message": "Emergency latch disarmed via Dashboard button"
            })
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status":"ok","message":"Simulated emergency disarmed"}')
        elif self.path.startswith('/api/simulate/trigger?type='):
            threat_type = self.path.split('=')[1]
            self.trigger_simulation(threat_type)
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
        else:
            self.send_response(404)
            self.end_headers()

    def trigger_simulation(self, trigger_type):
        t_str = time.strftime("%H:%M:%S")
        if trigger_type == "flame":
            sim_state["sensors"]["flame_detected"] = True
            sim_state["system"]["state"] = "CRITICAL"
            sim_state["system"]["threat"] = "OPTICAL_FLAME_DETECTED"
            sim_state["event_log"].append({"timestamp": t_str, "type": "CRITICAL", "message": "Optical Flame Sensor Triggered!"})
        elif trigger_type == "smoke":
            sim_state["sensors"]["smoke_raw"] = 1650
            sim_state["sensors"]["smoke_voltage"] = 2.21
            sim_state["system"]["state"] = "WARNING"
            sim_state["system"]["threat"] = "SMOKE_LEVEL_ELEVATED"
            sim_state["event_log"].append({"timestamp": t_str, "type": "WARNING", "message": "Smoke Level Exceeded Warning Threshold (1650 ADC)"})
        elif trigger_type == "dual_crit":
            sim_state["sensors"]["smoke_raw"] = 2400
            sim_state["sensors"]["smoke_voltage"] = 3.22
            sim_state["sensors"]["temperature"] = 52.8
            sim_state["system"]["state"] = "CRITICAL"
            sim_state["system"]["threat"] = "EXTREME_SMOKE_AND_HEAT"
            sim_state["event_log"].append({"timestamp": t_str, "type": "CRITICAL", "message": "Dual Threat Triggered: Extreme Smoke & Heat!"})
        elif trigger_type == "button":
            sim_state["sensors"]["emergency_pressed"] = True
            sim_state["system"]["state"] = "EMERGENCY"
            sim_state["system"]["threat"] = "MANUAL_PUSH_BUTTON"
            sim_state["event_log"].append({"timestamp": t_str, "type": "EMERGENCY", "message": "Manual Emergency Button Pressed (Latched)"})
        elif trigger_type == "clear":
            sim_state["sensors"]["flame_detected"] = False
            sim_state["sensors"]["emergency_pressed"] = False
            sim_state["sensors"]["smoke_raw"] = 580
            sim_state["sensors"]["smoke_voltage"] = 0.78
            sim_state["sensors"]["temperature"] = 27.2
            sim_state["system"]["state"] = "NORMAL"
            sim_state["system"]["threat"] = "NONE"
            sim_state["event_log"].append({"timestamp": t_str, "type": "NORMAL", "message": "All conditions restored to Normal baseline"})

    def handle_relay_esp32(self):
        # Relay live data from physical ESP32
        esp_ip = self.path.split('=')[1]
        url = f"http://{esp_ip}/api/status"
        try:
            req = urllib.request.Request(url, headers={'User-Agent': 'IoT-Dashboard-Bridge/1.0'})
            with urllib.request.urlopen(req, timeout=2.5) as response:
                content = response.read()
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(content)
        except Exception as e:
            self.send_response(502)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            err_json = json.dumps({"error": f"Failed to connect to ESP32 at {esp_ip}: {str(e)}"}).encode()
            self.wfile.write(err_json)

    def handle_get_status(self):
        # Update simulation counters if in normal state
        sim_state["system"]["uptime_seconds"] = int(time.time() - start_time)
        if sim_state["system"]["state"] == "NORMAL":
            # Add subtle realistic natural drift to temperature and smoke
            sim_state["sensors"]["temperature"] = round(26.8 + 0.4 * math.sin(time.time() / 15.0) + random.uniform(-0.1, 0.1), 1)
            sim_state["sensors"]["humidity"] = round(52.0 + 2.0 * math.cos(time.time() / 20.0), 0)
            sim_state["sensors"]["smoke_raw"] = int(580 + random.randint(-15, 20))
            sim_state["sensors"]["smoke_voltage"] = round((sim_state["sensors"]["smoke_raw"] / 4095.0 * 3.3) / 0.60, 2)

        payload = json.dumps(sim_state).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(payload)

def main():
    server_address = ('', PORT)
    httpd = HTTPServer(server_address, DashboardHandler)
    print(f"================================================================")
    print(f"  Smart Fire Detection Web Dashboard Server Running on Port {PORT}")
    print(f"  Local URL: http://localhost:{PORT}")
    print(f"  Directory: {DASHBOARD_DIR}")
    print(f"================================================================\n")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer shutting down gracefully.")
        httpd.server_close()

if __name__ == '__main__':
    main()

