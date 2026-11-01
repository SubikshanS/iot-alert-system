#ifndef MQ2_H
#define MQ2_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

class MQ2Sensor {
public:
    MQ2Sensor();

    void begin();
    void sample(SensorReadings& data);
    void calibrateBaseline(uint16_t sampleCount = 50);

    uint16_t getFilteredADC() const { return _filteredADC; }
    uint16_t getBaselineADC() const { return _baselineADC; }
    float getVoltage() const { return _currentVoltage; }
    bool isWarmedUp() const;
    bool isValid() const { return _isValid; }

private:
    static const uint8_t FILTER_WINDOW_SIZE = 10;
    uint16_t _sampleBuffer[FILTER_WINDOW_SIZE];
    uint8_t _bufferIndex;
    uint8_t _sampleCount;
    uint32_t _sampleSum;

    uint16_t _filteredADC;
    uint16_t _baselineADC;
    float _currentVoltage;
    bool _isValid;
    unsigned long _initTime_ms;

    void updateFilter(uint16_t rawReading);
};

#endif // MQ2_H

