#include "controllers/emergency.h"

EmergencyController::EmergencyController()
    : _isLatched(false),
      _lastPhysicalState(false),
      _debouncedState(false),
      _lastDebounceTime_ms(0),
      _pressStartTime_ms(0),
      _latchStartTime_ms(0),
      _longPressAcknowledged(false) {
}

void EmergencyController::begin() {
    pinMode(PIN_EMERGENCY_BTN, INPUT_PULLUP);
    _lastPhysicalState = (digitalRead(PIN_EMERGENCY_BTN) == LOW);
    _debouncedState = _lastPhysicalState;
    _isLatched = false;
    Serial.printf("[EMERGENCY] Button controller initialized on GPIO %d (Active LOW, Debounce: %lu ms).\n",
                  PIN_EMERGENCY_BTN, DEBOUNCE_DELAY_MS);
}

void EmergencyController::reset() {
    _isLatched = false;
    _latchStartTime_ms = 0;
    Serial.println("[EMERGENCY] System emergency state cleared/disarmed.");
}

void EmergencyController::manualTrigger() {
    if (!_isLatched) {
        _isLatched = true;
        _latchStartTime_ms = millis();
        Serial.println("[EMERGENCY] Manual trigger invoked programmatically.");
    }
}

unsigned long EmergencyController::getLatchedDuration_ms() const {
    return _isLatched ? (millis() - _latchStartTime_ms) : 0;
}

void EmergencyController::poll(SensorReadings& sensors, SystemStatus& status) {
    // Physical reading is LOW when button is pressed (to GND via internal pull-up)
    bool reading = (digitalRead(PIN_EMERGENCY_BTN) == LOW);

    // Debounce timer check
    if (reading != _lastPhysicalState) {
        _lastDebounceTime_ms = millis();
    }

    if ((millis() - _lastDebounceTime_ms) > DEBOUNCE_DELAY_MS) {
        if (reading != _debouncedState) {
            _debouncedState = reading;

            if (_debouncedState) {
                // Button Pressed Down Event
                _pressStartTime_ms = millis();
                _longPressAcknowledged = false;

                if (!_isLatched) {
                    _isLatched = true;
                    _latchStartTime_ms = millis();
                    Serial.println("\n*************************************************************");
                    Serial.println("  [EMERGENCY] !!! MANUAL EMERGENCY BUTTON TRIGGERED (LATCHED) !!!");
                    Serial.println("*************************************************************\n");
                }
            }
        }
    }

    // Long-Press Reset Detection:
    // If the button is held continuously for 3+ seconds while in emergency, disarm/reset
    if (_debouncedState && _isLatched) {
        if ((millis() - _pressStartTime_ms >= RESET_HOLD_TIME_MS) && !_longPressAcknowledged) {
            _longPressAcknowledged = true;
            reset();
            Serial.println("[EMERGENCY] Emergency latch cleared via 3-second physical button hold.");
        }
    }

    _lastPhysicalState = reading;

    // Propagate latched state to shared sensor telemetry
    sensors.emergency_pressed = _isLatched;
}

