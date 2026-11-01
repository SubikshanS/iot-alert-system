#ifndef FLAME_SENSOR_H
#define FLAME_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

class FlameSensor {
public:
    FlameSensor();

    void begin();
    void sample(SensorReadings& data);

    bool isFlameDetected() const { return _flameActive; }
    bool isValid() const { return _isValid; }
    uint32_t getTriggerCount() const { return _triggerCount; }

private:
    bool _flameActive;
    bool _isValid;
    uint32_t _triggerCount;
    uint8_t _consecutiveActiveReads;
    unsigned long _lastDetectionTime_ms;

    // Minimum time to hold flame active state to prevent rapid flapping from flickering flame
    static const unsigned long FLAME_HOLD_TIME_MS = 2500;
};

#endif // FLAME_SENSOR_H

