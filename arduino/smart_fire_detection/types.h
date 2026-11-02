#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

/**
 * @brief Discrete operational states of the detection system state machine.
 */
enum SystemState {
    STATE_BOOT = 0,         ///< System powering up, running self-test and hardware validation
    STATE_NORMAL,           ///< All sensors nominal, green status LED active
    STATE_WARNING,          ///< Elevated smoke OR high temperature detected; warning indicators active
    STATE_CRITICAL,         ///< Optical flame detected OR simultaneous extreme smoke & heat
    STATE_EMERGENCY,        ///< Manual emergency trigger activated by push button
    STATE_SENSOR_ERROR      ///< Hardware sensor disconnected, faulty, or reading out-of-spec
};

/**
 * @brief Primary causal threat triggering state transition.
 */
enum ThreatReason {
    THREAT_NONE = 0,
    THREAT_SMOKE_WARN,          ///< Smoke exceeded warning threshold
    THREAT_TEMP_WARN,           ///< Temperature exceeded warning threshold
    THREAT_FLAME_DETECTED,      ///< Optical infrared flame sensor triggered
    THREAT_DUAL_CRITICAL,       ///< Combined high smoke and high temperature
    THREAT_MANUAL_EMERGENCY,    ///< Physical button engaged
    THREAT_SENSOR_FAULT         ///< Sensor communication failure or electrical open/short
};

/**
 * @brief Unified container holding instantaneous and filtered sensor telemetry.
 */
struct SensorReadings {
    // Environmental
    float temperature;          ///< Temperature in degrees Celsius
    float humidity;             ///< Relative humidity percentage (0-100%)
    uint16_t smoke_raw;         ///< Raw 12-bit ADC reading (0 - 4095)
    float smoke_voltage;        ///< Reconstructed sensor output voltage (0.0V - 5.0V)
    bool flame_detected;        ///< Optical flame detected flag (normalized active HIGH)

    // Manual Input
    bool emergency_pressed;     ///< Emergency push button state (normalized active HIGH)

    // Hardware Health & Validation Flags
    bool dht_valid;             ///< True if DHT11 returned valid reading (not NaN)
    bool mq2_valid;             ///< True if MQ-2 reading is within valid electrical range
    bool flame_valid;           ///< True if flame sensor circuit is operating
    bool gps_valid;             ///< True if GPS has acquired a valid 2D/3D satellite fix

    // Geolocation & Timing
    double gps_latitude;        ///< Latitude in decimal degrees (e.g. 13.0827)
    double gps_longitude;       ///< Longitude in decimal degrees (e.g. 80.2707)
    uint8_t gps_satellites;     ///< Number of connected satellites
    float gps_altitude_m;       ///< Altitude above mean sea level in meters
    float gps_hdop;             ///< Horizontal Dilution of Precision
    char  gps_time_str[24];     ///< UTC formatted timestamp: "YYYY-MM-DD HH:MM:SS"

    // Telemetry timestamps
    unsigned long last_dht_read_ms;
    unsigned long last_mq2_read_ms;
};

/**
 * @brief Unified container holding system-wide operational status and health metrics.
 */
struct SystemStatus {
    SystemState currentState;
    SystemState previousState;
    ThreatReason activeThreat;

    // Connectivity
    bool wifiConnected;
    char ipAddress[16];
    int8_t wifiRSSI;

    // Timing and Rate Limiting
    unsigned long stateEnterTime_ms;
    unsigned long lastCriticalAlert_ms;
    unsigned long lastWarningAlert_ms;
    uint32_t uptimeSeconds;

    // System Diagnostics
    uint32_t freeHeapBytes;
    uint8_t resetReason;
};

#endif // TYPES_H
