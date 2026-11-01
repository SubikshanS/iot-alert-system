#ifndef EMERGENCY_CONTROLLER_H
#define EMERGENCY_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

class EmergencyController {
public:
    EmergencyController();

    void begin();
    
    /**
     * @brief Polls button pin, applies debouncing, handles latching and long-press reset.
     * @param sensors Reference to shared sensor data to update emergency_pressed flag.
     * @param status Reference to system status for state-dependent actions.
     */
    void poll(SensorReadings& sensors, SystemStatus& status);

    void reset();
    void manualTrigger();

    bool isLatched() const { return _isLatched; }
    unsigned long getLatchedDuration_ms() const;

private:
    static const unsigned long DEBOUNCE_DELAY_MS = 50;
    static const unsigned long RESET_HOLD_TIME_MS = 3000;

    bool _isLatched;
    bool _lastPhysicalState;
    bool _debouncedState;
    unsigned long _lastDebounceTime_ms;
    unsigned long _pressStartTime_ms;
    unsigned long _latchStartTime_ms;
    bool _longPressAcknowledged;
};

#endif // EMERGENCY_CONTROLLER_H

