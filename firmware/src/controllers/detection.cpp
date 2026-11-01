#include "controllers/detection.h"
#include "system_manager.h"

DetectionController::DetectionController()
    : _smokeWarnThreshold(SMOKE_WARN_THRESHOLD_ADC),
      _smokeCritThreshold(SMOKE_CRIT_THRESHOLD_ADC),
      _tempWarnThreshold(TEMP_WARN_THRESHOLD_C),
      _tempCritThreshold(TEMP_CRIT_THRESHOLD_C),
      _stateChanged(false) {
}

void DetectionController::begin() {
    Serial.println("[DETECTION] Multi-sensor threat evaluation engine initialized.");
    Serial.printf("[DETECTION] Thresholds -> Smoke Warn: %u ADC, Smoke Crit: %u ADC | Temp Warn: %.1f C, Temp Crit: %.1f C\n",
                  _smokeWarnThreshold, _smokeCritThreshold, _tempWarnThreshold, _tempCritThreshold);
}

SystemState DetectionController::determineState(const SensorReadings& sensors,
                                               SystemState currentState,
                                               ThreatReason& outThreat) {
    // -------------------------------------------------------------------------
    // PRIORITY 1: Manual Emergency Push Button
    // Immediate human override takes absolute precedence over automated sensors.
    // -------------------------------------------------------------------------
    if (sensors.emergency_pressed) {
        outThreat = THREAT_MANUAL_EMERGENCY;
        return STATE_EMERGENCY;
    }

    // If currently in emergency state and button is released, keep emergency
    // active until explicitly acknowledged or cleared
    if (currentState == STATE_EMERGENCY && sensors.emergency_pressed) {
        outThreat = THREAT_MANUAL_EMERGENCY;
        return STATE_EMERGENCY;
    }

    // -------------------------------------------------------------------------
    // PRIORITY 2: Optical Infrared Flame Detection
    // Life-safety principle: Optical flame detection is never ignored or downgraded
    // even if smoke or temperature readings are still at baseline.
    // -------------------------------------------------------------------------
    if (sensors.flame_detected) {
        outThreat = THREAT_FLAME_DETECTED;
        return STATE_CRITICAL;
    }

    // -------------------------------------------------------------------------
    // PRIORITY 3: Critical Environmental Threats (Severe Smoke OR Dual Elevated)
    // -------------------------------------------------------------------------
    bool severeSmoke = (sensors.smoke_raw >= _smokeCritThreshold);
    bool dualElevated = (sensors.smoke_raw >= _smokeWarnThreshold &&
                         sensors.temperature >= _tempWarnThreshold);

    if (severeSmoke || dualElevated) {
        outThreat = THREAT_DUAL_CRITICAL;
        return STATE_CRITICAL;
    }

    // If previously in CRITICAL, require values to drop below hysteresis threshold
    // before dropping down to WARNING
    if (currentState == STATE_CRITICAL) {
        uint16_t smokeCritClear = (_smokeCritThreshold > SMOKE_HYSTERESIS_ADC) ?
                                  (_smokeCritThreshold - SMOKE_HYSTERESIS_ADC) : 0;
        float tempCritClear = (_tempCritThreshold > TEMP_HYSTERESIS_C) ?
                              (_tempCritThreshold - TEMP_HYSTERESIS_C) : 0.0f;

        if (sensors.smoke_raw >= smokeCritClear || sensors.temperature >= tempCritClear) {
            outThreat = THREAT_DUAL_CRITICAL;
            return STATE_CRITICAL;
        }
    }

    // -------------------------------------------------------------------------
    // PRIORITY 4: Pre-Alarm Warnings (Elevated Smoke OR Elevated Heat) with Hysteresis
    // -------------------------------------------------------------------------
    uint16_t smokeWarnClear = (_smokeWarnThreshold > SMOKE_HYSTERESIS_ADC) ?
                              (_smokeWarnThreshold - SMOKE_HYSTERESIS_ADC) : 0;
    float tempWarnClear = (_tempWarnThreshold > TEMP_HYSTERESIS_C) ?
                          (_tempWarnThreshold - TEMP_HYSTERESIS_C) : 0.0f;

    if (currentState == STATE_WARNING) {
        // Evaluate hysteresis window to prevent rapid oscillation (flapping)
        bool smokeStillElevated = (sensors.smoke_raw >= smokeWarnClear);
        bool tempStillElevated = (sensors.temperature >= tempWarnClear);

        if (smokeStillElevated) {
            outThreat = THREAT_SMOKE_WARN;
            return STATE_WARNING;
        }
        if (tempStillElevated) {
            outThreat = THREAT_TEMP_WARN;
            return STATE_WARNING;
        }
    } else {
        // Entering WARNING from NORMAL or SENSOR_ERROR
        if (sensors.smoke_raw >= _smokeWarnThreshold) {
            outThreat = THREAT_SMOKE_WARN;
            return STATE_WARNING;
        }
        if (sensors.temperature >= _tempWarnThreshold) {
            outThreat = THREAT_TEMP_WARN;
            return STATE_WARNING;
        }
    }

    // -------------------------------------------------------------------------
    // PRIORITY 5: Sensor Health & Hardware Communication Faults
    // -------------------------------------------------------------------------
    if (!sensors.dht_valid || !sensors.mq2_valid) {
        outThreat = THREAT_SENSOR_FAULT;
        return STATE_SENSOR_ERROR;
    }

    // -------------------------------------------------------------------------
    // PRIORITY 6: Clean Baseline (All sensors nominal)
    // -------------------------------------------------------------------------
    outThreat = THREAT_NONE;
    return STATE_NORMAL;
}

void DetectionController::onStateTransition(SystemState from, SystemState to, ThreatReason threat) {
    Serial.println("\n-------------------------------------------------------------");
    Serial.printf("[STATE TRANSITION] >>> %s ===> %s <<<\n",
                  SystemManager::stateToString(from),
                  SystemManager::stateToString(to));
    Serial.printf("  Trigger Reason: %s\n", SystemManager::threatToString(threat));
    Serial.println("-------------------------------------------------------------\n");
}

void DetectionController::evaluate(const SensorReadings& sensors, SystemStatus& status) {
    ThreatReason detectedThreat = THREAT_NONE;
    SystemState nextState = determineState(sensors, status.currentState, detectedThreat);

    _stateChanged = (nextState != status.currentState);

    if (_stateChanged) {
        onStateTransition(status.currentState, nextState, detectedThreat);
        status.previousState = status.currentState;
        status.currentState = nextState;
        status.activeThreat = detectedThreat;
        status.stateEnterTime_ms = millis();
    } else {
        status.activeThreat = detectedThreat;
    }
}

