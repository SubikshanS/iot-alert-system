#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include "config.h"
#include "types.h"

class GPSSensor {
public:
    GPSSensor();

    void begin();
    void update(SensorReadings& data);

    bool hasValidFix() const;
    bool isHardwareConnected() const;

    double getLatitude() const;
    double getLongitude() const;
    uint8_t getSatellites() const;
    float getAltitudeMeters() const;
    float getHDOP() const;
    uint32_t getFixAge() const;

    String getGoogleMapsUrl() const;
    String getFormattedCoordinates() const;
    String getUtcTimestamp() const;

private:
    TinyGPSPlus _gps;
    HardwareSerial _gpsSerial;

    bool _hasFix;
    bool _hardwareConnected;
    uint32_t _charsProcessed;
    unsigned long _lastByteReceivedTime_ms;
    unsigned long _lastFixLogTime_ms;

    void updateTelemetryData(SensorReadings& data);
};

#endif // GPS_SENSOR_H
