#include "sensors/mq2.h"

MQ2Sensor::MQ2Sensor()
    : _bufferIndex(0),
      _sampleCount(0),
      _sampleSum(0),
      _filteredADC(0),
      _baselineADC(SMOKE_BASELINE_ADC),
      _currentVoltage(0.0f),
      _isValid(false),
      _initTime_ms(0) {
    for (uint8_t i = 0; i < FILTER_WINDOW_SIZE; i++) {
        _sampleBuffer[i] = 0;
    }
}

void MQ2Sensor::begin() {
    pinMode(PIN_MQ2_ADC, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_MQ2_ADC, ADC_11db);

    _initTime_ms = millis();

    // Prime the filter buffer with initial reads
    for (uint8_t i = 0; i < FILTER_WINDOW_SIZE; i++) {
        uint16_t initialVal = analogRead(PIN_MQ2_ADC);
        updateFilter(initialVal);
        delay(10);
    }

    _isValid = (_filteredADC >= 40 && _filteredADC <= 4080);
    Serial.printf("[MQ2] Initialized on GPIO %d. Initial Filtered ADC: %u, Baseline: %u\n",
                  PIN_MQ2_ADC, _filteredADC, _baselineADC);
}

void MQ2Sensor::updateFilter(uint16_t rawReading) {
    if (_sampleCount < FILTER_WINDOW_SIZE) {
        _sampleBuffer[_bufferIndex] = rawReading;
        _sampleSum += rawReading;
        _sampleCount++;
        _bufferIndex = (_bufferIndex + 1) % FILTER_WINDOW_SIZE;
        _filteredADC = _sampleSum / _sampleCount;
    } else {
        _sampleSum -= _sampleBuffer[_bufferIndex];
        _sampleBuffer[_bufferIndex] = rawReading;
        _sampleSum += rawReading;
        _bufferIndex = (_bufferIndex + 1) % FILTER_WINDOW_SIZE;
        _filteredADC = _sampleSum / FILTER_WINDOW_SIZE;
    }
}

void MQ2Sensor::sample(SensorReadings& data) {
    uint16_t raw = analogRead(PIN_MQ2_ADC);
    updateFilter(raw);

    // Calculate voltage at ESP32 pin (3.3V reference at 11dB attenuation)
    float pinVoltage = ((float)_filteredADC / 4095.0f) * 3.3f;

    // Reconstruct true MQ-2 AOUT voltage prior to the 2.2k/3.3k divider
    _currentVoltage = pinVoltage / MQ2_VOLTAGE_DIVIDER_RATIO;

    // Validate electrical sanity bounds
    // Below 40 indicates disconnected wire/floating pin; above 4080 indicates direct short to rail
    _isValid = (raw >= 40 && raw <= 4085);

    // Populate shared telemetry
    data.smoke_raw = _filteredADC;
    data.smoke_voltage = _currentVoltage;
    data.mq2_valid = _isValid;
    data.last_mq2_read_ms = millis();
}

void MQ2Sensor::calibrateBaseline(uint16_t sampleCount) {
    Serial.println("[MQ2] Commencing baseline calibration in clean air...");
    uint32_t sum = 0;
    uint16_t validCount = 0;

    for (uint16_t i = 0; i < sampleCount; i++) {
        uint16_t val = analogRead(PIN_MQ2_ADC);
        if (val >= 40 && val <= 4080) {
            sum += val;
            validCount++;
        }
        delay(20);
    }

    if (validCount > (sampleCount / 2)) {
        _baselineADC = sum / validCount;
        Serial.printf("[MQ2] Baseline calibration complete: %u ADC counts.\n", _baselineADC);
    } else {
        Serial.println("[MQ2] WARNING: Baseline calibration failed due to noisy/invalid readings. Retaining default.");
    }
}

bool MQ2Sensor::isWarmedUp() const {
    // Standard quick warm-up threshold for demonstration: 20 seconds
    return (millis() - _initTime_ms >= 20000UL);
}

