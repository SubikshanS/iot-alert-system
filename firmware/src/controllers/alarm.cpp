#include "controllers/alarm.h"

AlarmController::AlarmController()
    : _buzzerState(false),
      _redLedState(false),
      _greenLedState(true),
      _lastBuzzerStep_ms(0),
      _lastLedStep_ms(0),
      _patternPhase(0) {
}

void AlarmController::begin() {
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);

    // Initial safe states: Green ON, Red OFF, Buzzer OFF
    setOutputs(true, false, false);
    Serial.println("[ALARM] Local visual & audible alarm controller initialized.");
}

void AlarmController::setOutputs(bool green, bool red, bool buzzer) {
    if (_greenLedState != green) {
        _greenLedState = green;
        digitalWrite(PIN_LED_GREEN, green ? HIGH : LOW);
    }
    if (_redLedState != red) {
        _redLedState = red;
        digitalWrite(PIN_LED_RED, red ? HIGH : LOW);
    }
    if (_buzzerState != buzzer) {
        _buzzerState = buzzer;
        digitalWrite(PIN_BUZZER, buzzer ? HIGH : LOW);
    }
}

void AlarmController::silence() {
    setOutputs(_greenLedState, false, false);
}

void AlarmController::handleNormalPattern() {
    // Solid Green, Red OFF, Buzzer OFF
    setOutputs(true, false, false);
}

void AlarmController::handleWarningPattern() {
    unsigned long now = millis();

    // Red LED: Blinks 500ms ON / 500ms OFF
    bool redState = ((now / 500UL) % 2 == 0);

    // Buzzer: Intermittent advisory chirp: 150ms ON every 2000ms
    unsigned long buzzerCycle = now % 2000UL;
    bool buzzerState = (buzzerCycle < 150UL);

    setOutputs(false, redState, buzzerState);
}

void AlarmController::handleCriticalPattern() {
    unsigned long now = millis();

    // Red LED: Rapid 150ms Strobe
    bool redState = ((now / 150UL) % 2 == 0);

    // Buzzer: Urgent alarm cadence: 200ms ON / 200ms OFF
    bool buzzerState = ((now / 200UL) % 2 == 0);

    setOutputs(false, redState, buzzerState);
}

void AlarmController::handleEmergencyPattern() {
    unsigned long now = millis();

    // Red LED: Solid ON (or high-speed 100ms strobe)
    bool redState = true;

    // Buzzer: Piercing continuous tone (or fast warble 100ms ON / 100ms OFF)
    bool buzzerState = ((now / 120UL) % 2 == 0);

    setOutputs(false, redState, buzzerState);
}

void AlarmController::handleSensorErrorPattern() {
    unsigned long now = millis();

    // Green LED: Slow toggle
    bool greenState = ((now / 1000UL) % 2 == 0);

    // Red LED: Double-pulse diagnostic blink
    unsigned long subCycle = now % 1500UL;
    bool redState = (subCycle < 100UL || (subCycle >= 200UL && subCycle < 300UL));

    // Buzzer: Brief chirp every 3 seconds
    unsigned long buzzCycle = now % 3000UL;
    bool buzzerState = (buzzCycle < 60UL);

    setOutputs(greenState, redState, buzzerState);
}

void AlarmController::update(SystemState state) {
    switch (state) {
        case STATE_NORMAL:
            handleNormalPattern();
            break;
        case STATE_WARNING:
            handleWarningPattern();
            break;
        case STATE_CRITICAL:
            handleCriticalPattern();
            break;
        case STATE_EMERGENCY:
            handleEmergencyPattern();
            break;
        case STATE_SENSOR_ERROR:
            handleSensorErrorPattern();
            break;
        case STATE_BOOT:
        default:
            setOutputs(true, false, false);
            break;
    }
}

