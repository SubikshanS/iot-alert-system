#ifndef ALARM_CONTROLLER_H
#define ALARM_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

class AlarmController {
public:
    AlarmController();

    void begin();
    
    /**
     * @brief Updates LED and Buzzer outputs according to active system state.
     * Non-blocking, must be called regularly in the main loop.
     */
    void update(SystemState state);

    void silence();

    bool isBuzzerActive() const   { return _buzzerState; }
    bool isRedLedActive() const    { return _redLedState; }
    bool isGreenLedActive() const  { return _greenLedState; }

private:
    bool _buzzerState;
    bool _redLedState;
    bool _greenLedState;

    unsigned long _lastBuzzerStep_ms;
    unsigned long _lastLedStep_ms;
    uint8_t       _patternPhase;

    void setOutputs(bool green, bool red, bool buzzer);
    void handleNormalPattern();
    void handleWarningPattern();
    void handleCriticalPattern();
    void handleEmergencyPattern();
    void handleSensorErrorPattern();
};

#endif // ALARM_CONTROLLER_H

