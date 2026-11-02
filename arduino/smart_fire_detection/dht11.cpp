#include "dht11.h"

DHTSensor::DHTSensor()
    : _dht(PIN_DHT11, DHT11),
      _lastValidTemp(25.0f),
      _lastValidHumidity(50.0f),
      _isValid(false),
      _consecutiveErrors(0),
      _lastAttemptTime_ms(0) {
}

void DHTSensor::begin() {
    _dht.begin();
    Serial.printf("[DHT11] Initialized on GPIO %d. Ready for single-wire telemetry.\n", PIN_DHT11);
    
    // Perform initial read attempt
    delay(50);
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    if (!isnan(t) && !isnan(h) && t >= 0.0f && t <= 60.0f) {
        _lastValidTemp = t;
        _lastValidHumidity = h;
        _isValid = true;
        Serial.printf("[DHT11] Initial reading valid: Temp=%.1f C, Humidity=%.1f %%\n", t, h);
    } else {
        Serial.println("[DHT11] Initial reading pending (sensor warming up or awaiting first bus cycle).");
    }
}

void DHTSensor::sample(SensorReadings& data) {
    _lastAttemptTime_ms = millis();
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    // Check for standard 1-wire timing/checksum failure or physical disconnection
    bool readFailed = (isnan(t) || isnan(h) || t < 0.0f || t > 60.0f || h < 15.0f || h > 98.0f);

    if (readFailed) {
        if (_consecutiveErrors < 255) {
            _consecutiveErrors++;
        }
        
        // If 3 consecutive 2-second reads fail (6 seconds with no signal), declare sensor invalid
        if (_consecutiveErrors >= 3) {
            _isValid = false;
            Serial.printf("[DHT11] WARNING: Consecutive read failures (%u). Check wiring on GPIO %d.\n",
                          _consecutiveErrors, PIN_DHT11);
        }
    } else {
        _consecutiveErrors = 0;
        _isValid = true;
        _lastValidTemp = t;
        _lastValidHumidity = h;
    }

    // Populate shared telemetry (retains last known good value during transient single drops)
    data.temperature = _lastValidTemp;
    data.humidity = _lastValidHumidity;
    data.dht_valid = _isValid;
    data.last_dht_read_ms = _lastAttemptTime_ms;
}

