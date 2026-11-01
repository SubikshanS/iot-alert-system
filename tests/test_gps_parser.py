#!/usr/bin/env python3
"""
NEO-6M GPS NMEA Parser & Geolocation Telemetry Verification Test Suite
Verifies NMEA stream parsing, coordinate validity, dilution of precision, and Google Maps generation.
"""

import math

class MockGPSParser:
    def __init__(self):
        self.is_valid = False
        self.latitude = 0.0
        self.longitude = 0.0
        self.satellites = 0
        self.altitude = 0.0
        self.hdop = 99.9
        self.chars_processed = 0

    def parse_gga(self, sentence: str):
        # Format: $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
        self.chars_processed += len(sentence)
        parts = sentence.strip().split(',')
        if len(parts) < 10 or parts[0] != "$GPGGA":
            return

        fix_quality = int(parts[6]) if parts[6] else 0
        sat_count = int(parts[7]) if parts[7] else 0
        hdop = float(parts[8]) if parts[8] else 99.9
        alt = float(parts[9]) if parts[9] else 0.0

        self.satellites = sat_count
        self.hdop = hdop
        self.altitude = alt

        # Valid fix requires fix_quality >= 1 and at least 3 satellites
        if fix_quality >= 1 and sat_count >= 3 and parts[2] and parts[4]:
            # Convert NMEA ddmm.mmmm to decimal degrees
            raw_lat = float(parts[2])
            lat_deg = math.floor(raw_lat / 100)
            lat_min = raw_lat - (lat_deg * 100)
            lat = lat_deg + (lat_min / 60.0)
            if parts[3] == 'S':
                lat = -lat

            raw_lng = float(parts[4])
            lng_deg = math.floor(raw_lng / 100)
            lng_min = raw_lng - (lng_deg * 100)
            lng = lng_deg + (lng_min / 60.0)
            if parts[5] == 'W':
                lng = -lng

            self.latitude = lat
            self.longitude = lng
            self.is_valid = True
        else:
            self.latitude = 0.0
            self.longitude = 0.0
            self.is_valid = False

    def get_google_maps_url(self) -> str:
        if self.is_valid:
            return f"https://maps.google.com/?q={self.latitude:.6f},{self.longitude:.6f}"
        return "Location Unavailable (Indoor Prototype / No GPS Fix)"

    def get_formatted_coords(self) -> str:
        if self.is_valid:
            lat_dir = 'N' if self.latitude >= 0 else 'S'
            lng_dir = 'E' if self.longitude >= 0 else 'W'
            return f"{abs(self.latitude):.6f} {lat_dir}, {abs(self.longitude):.6f} {lng_dir}"
        return "Searching for Satellites"


def run_tests():
    print("[TEST SUITE] Running GPS Parser & Geolocation Logic Tests...")
    gps = MockGPSParser()

    # 1. Test Indoor / No Satellite Lock ($GPGGA fix quality = 0, 0 sats)
    indoor_gga = "$GPGGA,123519,0000.0000,N,00000.0000,E,0,00,99.9,0.0,M,0.0,M,,*66"
    gps.parse_gga(indoor_gga)
    assert not gps.is_valid, "Indoor reading incorrectly marked valid!"
    assert gps.latitude == 0.0 and gps.longitude == 0.0, "Indoor coordinates should be 0.0"
    assert "Location Unavailable" in gps.get_google_maps_url(), "Expected unavailable URL"
    print("  [PASS] Test 1: Indoor unacquired satellite state safely handled without fake data")

    # 2. Test Partial Satellites (< 3 sats, fix_quality = 0)
    partial_gga = "$GPGGA,123520,1304.9620,N,08016.2420,E,0,02,12.5,15.2,M,-2.5,M,,*71"
    gps.parse_gga(partial_gga)
    assert not gps.is_valid, "2 satellites should not constitute a valid 2D/3D fix"
    print("  [PASS] Test 2: Inadequate satellite count (2 sats) correctly rejected")

    # 3. Test Valid Outdoor Satellite Fix (Fix quality = 1, 8 sats, HDOP = 0.9)
    # Chennai / Central: 1304.9620 N (13° 04.962' N = 13.0827° N), 08016.2420 E (80° 16.242' E = 80.2707° E)
    valid_gga = "$GPGGA,123525,1304.9620,N,08016.2420,E,1,08,0.9,14.5,M,-2.5,M,,*78"
    gps.parse_gga(valid_gga)
    assert gps.is_valid, "Valid fix failed to parse!"
    assert abs(gps.latitude - 13.0827) < 0.001, f"Latitude parse error: {gps.latitude}"
    assert abs(gps.longitude - 80.2707) < 0.001, f"Longitude parse error: {gps.longitude}"
    assert gps.satellites == 8
    assert gps.hdop == 0.9
    assert gps.get_google_maps_url() == "https://maps.google.com/?q=13.082700,80.270700"
    print("  [PASS] Test 3: Valid outdoor 3D fix parsed and formatted to Google Maps URL")

    # 4. Test Formatted Coordinates
    formatted = gps.get_formatted_coords()
    assert "13.082700 N, 80.270700 E" in formatted
    print("  [PASS] Test 4: Formatted coordinate string generated correctly")

    print("[TEST SUITE] All GPS Logic and Safety Tests Passed Successfully!\n")

if __name__ == "__main__":
    run_tests()

