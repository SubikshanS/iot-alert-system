#ifndef DETECTION_CONTROLLER_H
#define DETECTION_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

class DetectionController {
public:
    DetectionController();

    void begin();
    
    /**
     * @brief Core sensor fusion evaluation algorithm.
     * Evaluates fused sensor readings and drives deterministic state machine transitions.
     * 
     * @param sensors Constant reference to current sensor readings.
     * @param status Reference to system status to update state and active threat.
     */
    void evaluate(const SensorReadings& sensors, SystemStatus& status);

    // Dynamic Threshold Configuration
    void setSmokeWarnThreshold(uint16_t val) { _smokeWarnThreshold = val; }
    void setSmokeCritThreshold(uint16_t val) { _smokeCritThreshold = val; }
    void setTempWarnThreshold(float val)     { _tempWarnThreshold = val; }
    void setTempCritThreshold(float val)     { _tempCritThreshold = val; }

    uint16_t getSmokeWarnThreshold() const { return _smokeWarnThreshold; }
    uint16_t getSmokeCritThreshold() const { return _smokeCritThreshold; }
    float    getTempWarnThreshold() const  { return _tempWarnThreshold; }
    float    getTempCritThreshold() const  { return _tempCritThreshold; }

    bool stateChangedThisCycle() const { return _stateChanged; }

private:
    uint16_t _smokeWarnThreshold;
    uint16_t _smokeCritThreshold;
    float    _tempWarnThreshold;
    float    _tempCritThreshold;

    bool     _stateChanged;

    SystemState determineState(const SensorReadings& sensors,
                               SystemState currentState,
                               ThreatReason& outThreat);

    void onStateTransition(SystemState from, SystemState to, ThreatReason threat);
};

#endif // DETECTION_CONTROLLER_H

