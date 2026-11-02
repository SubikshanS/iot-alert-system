#include "flame.h"

FlameSensor::FlameSensor()
    : _flameActive(false),
      _isValid(true),
      _triggerCount(0),
      _consecutiveActiveReads(0),
      _lastDetectionTime_ms(0) {
}

void FlameSensor::begin() {
    pinMode(PIN_FLAME_DO, INPUT);
    _isValid = true;
    _flameActive = false;
    Serial.printf("[FLAME] Optical IR sensor initialized on GPIO %d (Active LOW logic).\n", PIN_FLAME_DO);
}

void FlameSensor::sample(SensorReadings& data) {
    // LM393 comparator outputs LOW (0V) when IR flame radiation is sensed
    bool pinIsLow = (digitalRead(PIN_FLAME_DO) == LOW);

    if (pinIsLow) {
        if (_consecutiveActiveReads < 255) {
            _consecutiveActiveReads++;
        }

        // Require 2 consecutive active samples (~100ms) to filter transient EMI or lamp flicker
        if (_consecutiveActiveReads >= 2) {
            if (!_flameActive) {
                _triggerCount++;
                Serial.printf("[FLAME] !!! OPTICAL FLAME DETECTED on GPIO %d (Event #%u) !!!\n",
                              PIN_FLAME_DO, _triggerCount);
            }
            _flameActive = true;
            _lastDetectionTime_ms = millis();
        }
    } else {
        _consecutiveActiveReads = 0;

        // Apply hysteresis/hold-down window: retain active state for FLAME_HOLD_TIME_MS
        // to prevent rapid oscillation when a flame flickers
        if (_flameActive && (millis() - _lastDetectionTime_ms > FLAME_HOLD_TIME_MS)) {
            _flameActive = false;
            Serial.println("[FLAME] Optical flame signature cleared (Radiation ceased).");
        }
    }

    // Update shared telemetry container
    data.flame_detected = _flameActive;
    data.flame_valid = _isValid;
}

