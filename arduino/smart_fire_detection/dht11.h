#ifndef DHT11_SENSOR_H
#define DHT11_SENSOR_H

#include <Arduino.h>
#include <DHT.h>
#include "config.h"
#include "types.h"

class DHTSensor {
public:
    DHTSensor();

    void begin();
    void sample(SensorReadings& data);

    float getTemperature() const { return _lastValidTemp; }
    float getHumidity() const { return _lastValidHumidity; }
    bool isValid() const { return _isValid; }
    uint8_t getConsecutiveErrors() const { return _consecutiveErrors; }

private:
    DHT _dht;
    float _lastValidTemp;
    float _lastValidHumidity;
    bool _isValid;
    uint8_t _consecutiveErrors;
    unsigned long _lastAttemptTime_ms;
};

#endif // DHT11_SENSOR_H

