#include "sensors/gps.h"

GPSSensor::GPSSensor()
    : _gpsSerial(2),
      _hasFix(false),
      _hardwareConnected(false),
      _charsProcessed(0),
      _lastByteReceivedTime_ms(0),
      _lastFixLogTime_ms(0) {
}

void GPSSensor::begin() {
    // Initialize HardwareSerial 2 on RX=GPIO16, TX=GPIO17 at 9600 baud
    _gpsSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    Serial.printf("[GPS] NEO-6M initialized on UART2 (RX: GPIO %d, TX: GPIO %d, Baud: %d)\n",
                  PIN_GPS_RX, PIN_GPS_TX, GPS_BAUD_RATE);
}

void GPSSensor::updateTelemetryData(SensorReadings& data) {
    bool validFix = (_gps.location.isValid() &&
                     _gps.location.age() < 5000 &&
                     _gps.satellites.isValid() &&
                     _gps.satellites.value() >= 3);

    _hasFix = validFix;
    data.gps_valid = validFix;

    if (validFix) {
        data.gps_latitude = _gps.location.lat();
        data.gps_longitude = _gps.location.lng();
        data.gps_satellites = _gps.satellites.value();
        data.gps_altitude_m = _gps.altitude.isValid() ? _gps.altitude.meters() : 0.0f;
        data.gps_hdop = _gps.hdop.isValid() ? _gps.hdop.hdop() : 99.9f;

        // Populate formatted UTC timestamp if available
        if (_gps.date.isValid() && _gps.time.isValid()) {
            snprintf(data.gps_time_str, sizeof(data.gps_time_str), "%04d-%02d-%02d %02d:%02d:%02d",
                     _gps.date.year(), _gps.date.month(), _gps.date.day(),
                     _gps.time.hour(), _gps.time.minute(), _gps.time.second());
        } else {
            strcpy(data.gps_time_str, "Awaiting Time Lock");
        }

        // Periodic location diagnostics
        if (millis() - _lastFixLogTime_ms >= 30000UL) {
            _lastFixLogTime_ms = millis();
            Serial.printf("[GPS] Satellite Fix Active: %s | Sats: %u | HDOP: %.1f | URL: %s\n",
                          getFormattedCoordinates().c_str(),
                          data.gps_satellites,
                          data.gps_hdop,
                          getGoogleMapsUrl().c_str());
        }
    } else {
        // STRICT SAFETY RULE: When satellite fix is unavailable indoors, NEVER fabricate false coordinates
        data.gps_latitude = 0.0;
        data.gps_longitude = 0.0;
        data.gps_satellites = _gps.satellites.isValid() ? _gps.satellites.value() : 0;
        data.gps_altitude_m = 0.0f;
        data.gps_hdop = 99.9f;
        strcpy(data.gps_time_str, "Indoor / No Lock");
    }
}

void GPSSensor::update(SensorReadings& data) {
    // Ingest all pending characters from UART FIFO into TinyGPSPlus parser
    while (_gpsSerial.available() > 0) {
        char c = (char)_gpsSerial.read();
        _gps.encode(c);
        _charsProcessed++;
        _lastByteReceivedTime_ms = millis();
    }

    // Hardware connection verification:
    if (_charsProcessed > 30 && (millis() - _lastByteReceivedTime_ms < 10000UL)) {
        _hardwareConnected = true;
    } else if (millis() > 10000UL && _charsProcessed < 10) {
        _hardwareConnected = false;
    }

    updateTelemetryData(data);
}

bool GPSSensor::hasValidFix() const {
    return _hasFix;
}

bool GPSSensor::isHardwareConnected() const {
    return _hardwareConnected;
}

double GPSSensor::getLatitude() const {
    return _hasFix ? _gps.location.lat() : 0.0;
}

double GPSSensor::getLongitude() const {
    return _hasFix ? _gps.location.lng() : 0.0;
}

uint8_t GPSSensor::getSatellites() const {
    return _gps.satellites.isValid() ? _gps.satellites.value() : 0;
}

float GPSSensor::getAltitudeMeters() const {
    return (_hasFix && _gps.altitude.isValid()) ? _gps.altitude.meters() : 0.0f;
}

float GPSSensor::getHDOP() const {
    return (_hasFix && _gps.hdop.isValid()) ? _gps.hdop.hdop() : 99.9f;
}

uint32_t GPSSensor::getFixAge() const {
    return _gps.location.isValid() ? _gps.location.age() : 0xFFFFFFFF;
}

String GPSSensor::getGoogleMapsUrl() const {
    if (_hasFix) {
        char buf[80];
        snprintf(buf, sizeof(buf), "https://maps.google.com/?q=%.6f,%.6f",
                 _gps.location.lat(), _gps.location.lng());
        return String(buf);
    }
    return String("Location Unavailable (Indoor Prototype / No GPS Fix)");
}

String GPSSensor::getFormattedCoordinates() const {
    if (_hasFix) {
        char buf[64];
        char latDir = (_gps.location.lat() >= 0) ? 'N' : 'S';
        char lngDir = (_gps.location.lng() >= 0) ? 'E' : 'W';
        snprintf(buf, sizeof(buf), "%.6f %c, %.6f %c",
                 fabs(_gps.location.lat()), latDir,
                 fabs(_gps.location.lng()), lngDir);
        return String(buf);
    }
    return String("Searching for Satellites");
}

String GPSSensor::getUtcTimestamp() const {
    if (_gps.date.isValid() && _gps.time.isValid()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d UTC",
                 _gps.date.year(), _gps.date.month(), _gps.date.day(),
                 _gps.time.hour(), _gps.time.minute(), _gps.time.second());
        return String(buf);
    }
    return String("Time Not Synchronized");
}
