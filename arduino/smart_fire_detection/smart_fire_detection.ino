#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "system_manager.h"
#include "mq2.h"
#include "dht11.h"
#include "flame.h"
#include "gps.h"
#include "detection.h"
#include "alarm.h"
#include "emergency.h"
#include "oled.h"
#include "wifi_manager.h"
#include "telegram_manager.h"
#include "web_server.h"

// =============================================================================
// GLOBAL SENSOR DRIVERS
// =============================================================================
MQ2Sensor   mq2Sensor;
DHTSensor   dhtSensor;
FlameSensor flameSensor;
GPSSensor   gpsSensor;

// =============================================================================
// CONTROLLERS & DISPLAY
// =============================================================================
DetectionController  detectionController;
AlarmController      alarmController;
EmergencyController  emergencyController;
OLEDDisplay          oledDisplay;

// =============================================================================
// COMMUNICATION SERVICES & REST API
// =============================================================================
WiFiManager      wifiManager;
TelegramManager  telegramManager;
DeviceWebServer  webServer;

// =============================================================================
// GLOBAL SYSTEM STATE & TELEMETRY
// =============================================================================
SensorReadings sensorData;
SystemStatus systemStatus;

// =============================================================================
// NON-BLOCKING TASK TIMERS (millis() tracking)
// =============================================================================
static unsigned long lastButtonPollTime   = 0;
static unsigned long lastFlamePollTime    = 0;
static unsigned long lastMQ2SampleTime    = 0;
static unsigned long lastDHTSampleTime    = 0;
static unsigned long lastThreatEvalTime   = 0;
static unsigned long lastOLEDRefreshTime  = 0;
static unsigned long lastWiFiCheckTime    = 0;
static unsigned long lastTelegramTime     = 0;
static unsigned long lastDiagnosticsTime  = 0;

// =============================================================================
// SETUP - HARDWARE & SUBSYSTEM INITIALIZATION
// =============================================================================
void setup() {
    // 1. Initialize Serial Interface for Debug & Logging
    Serial.begin(DEBUG_SERIAL_BAUD);
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 1500)) {
        // Wait briefly for USB Serial interface to enumerate
    }

    // 2. Initialize Core Hardware Pin Modes and Default States
    SystemManager::initHardware();

    // 3. Print Detailed Boot Information & Diagnostics
    SystemManager::printBootDiagnostics();

    // 4. Initialize Global State Structures
    systemStatus.currentState = STATE_BOOT;
    systemStatus.previousState = STATE_BOOT;
    systemStatus.activeThreat = THREAT_NONE;
    systemStatus.wifiConnected = false;
    strcpy(systemStatus.ipAddress, "0.0.0.0");
    systemStatus.wifiRSSI = 0;
    systemStatus.stateEnterTime_ms = millis();
    systemStatus.lastCriticalAlert_ms = 0;
    systemStatus.lastWarningAlert_ms = 0;
    systemStatus.uptimeSeconds = 0;
    systemStatus.freeHeapBytes = ESP.getFreeHeap();

    // Initialize Sensor Data Defaults
    sensorData.temperature = 0.0f;
    sensorData.humidity = 0.0f;
    sensorData.smoke_raw = 0;
    sensorData.smoke_voltage = 0.0f;
    sensorData.flame_detected = false;
    sensorData.emergency_pressed = false;
    sensorData.dht_valid = false;
    sensorData.mq2_valid = false;
    sensorData.flame_valid = false;
    sensorData.gps_valid = false;
    sensorData.gps_latitude = 0.0;
    sensorData.gps_longitude = 0.0;
    sensorData.gps_satellites = 0;
    sensorData.gps_altitude_m = 0.0f;
    sensorData.gps_hdop = 99.9f;
    strcpy(sensorData.gps_time_str, "Booting");
    sensorData.last_dht_read_ms = 0;
    sensorData.last_mq2_read_ms = 0;

    // 5. Initialize Display (shows boot splash)
    oledDisplay.begin();

    // 6. Initialize Individual Sensor Subsystems
    Serial.println("[SETUP] Initializing individual sensor modules...");
    mq2Sensor.begin();
    dhtSensor.begin();
    flameSensor.begin();
    gpsSensor.begin();

    // 7. Initialize Actuator Controllers
    alarmController.begin();
    emergencyController.begin();
    detectionController.begin();

    // 8. Initialize Communication Services & REST API Server
    wifiManager.begin();
    wifiManager.checkConnection(systemStatus);
    telegramManager.begin();
    webServer.begin(&detectionController, &emergencyController);

    if (systemStatus.wifiConnected) {
        telegramManager.sendBootNotification(systemStatus.ipAddress);
    }

    // 9. Transition to STATE_NORMAL
    systemStatus.currentState = STATE_NORMAL;
    systemStatus.previousState = STATE_BOOT;
    systemStatus.stateEnterTime_ms = millis();

    // 10. Arm Hardware Task Watchdog Timer (8 seconds)
    SystemManager::initWatchdog(8);

    Serial.println("[SYSTEM] Subsystems, sensors, display, REST API, and telemetry services operational.\n");
}

// =============================================================================
// MAIN EXECUTION LOOP - COOPERATIVE TIME-SLICED SCHEDULER
// =============================================================================
void loop() {
    unsigned long currentMillis = millis();

    // 1. Kick the Task Watchdog to prevent reboot
    SystemManager::feedWatchdog();

    // 2. High-Frequency: Stream GPS NMEA characters from UART FIFO
    gpsSensor.update(sensorData);

    // 3. High-Frequency: Handle incoming HTTP REST API requests
    webServer.handleClient(sensorData, systemStatus);

    // 4. High-Frequency: Emergency Button Debounce, Latching & Long-Press Reset (20 ms)
    if (currentMillis - lastButtonPollTime >= INTERVAL_BUTTON_POLL_MS) {
        lastButtonPollTime = currentMillis;
        emergencyController.poll(sensorData, systemStatus);
    }

    // 5. High-Frequency Task: Optical Flame Sensor Poll (50 ms)
    if (currentMillis - lastFlamePollTime >= INTERVAL_FLAME_POLL_MS) {
        lastFlamePollTime = currentMillis;
        flameSensor.sample(sensorData);
    }

    // 6. Medium-Frequency Task: MQ-2 Analog Oversampling & Filter (100 ms)
    if (currentMillis - lastMQ2SampleTime >= INTERVAL_MQ2_SAMPLE_MS) {
        lastMQ2SampleTime = currentMillis;
        mq2Sensor.sample(sensorData);
    }

    // 7. Threat Evaluation & State Machine Update (100 ms)
    if (currentMillis - lastThreatEvalTime >= INTERVAL_THREAT_EVAL_MS) {
        lastThreatEvalTime = currentMillis;
        detectionController.evaluate(sensorData, systemStatus);
        // Dispatch notifications if threat status warrants alerting
        telegramManager.dispatchAlerts(sensorData, systemStatus);
    }

    // 8. Continuous Non-Blocking Alarm Cadence Update (LEDs and Buzzer)
    alarmController.update(systemStatus.currentState);

    // 9. Medium-Frequency Task: OLED Display Refresh (250 ms)
    if (currentMillis - lastOLEDRefreshTime >= INTERVAL_OLED_REFRESH_MS) {
        lastOLEDRefreshTime = currentMillis;
        oledDisplay.update(sensorData, systemStatus);
    }

    // 10. Low-Frequency Task: DHT11 Temperature & Humidity Ingestion (2000 ms)
    if (currentMillis - lastDHTSampleTime >= INTERVAL_DHT11_SAMPLE_MS) {
        lastDHTSampleTime = currentMillis;
        dhtSensor.sample(sensorData);
    }

    // 11. Low-Frequency Task: Wi-Fi Reconnection & Health Monitor (5000 ms)
    if (currentMillis - lastWiFiCheckTime >= INTERVAL_WIFI_CHECK_MS) {
        lastWiFiCheckTime = currentMillis;
        wifiManager.checkConnection(systemStatus);
    }

    // 12. Low-Frequency Task: Telegram Notification Queue Worker (1000 ms)
    if (currentMillis - lastTelegramTime >= INTERVAL_TELEGRAM_WORKER_MS) {
        lastTelegramTime = currentMillis;
        telegramManager.processQueue();
    }

    // 13. Periodic Heartbeat & Telemetry Summary Log (every 10 seconds)
    if (currentMillis - lastDiagnosticsTime >= 10000UL) {
        lastDiagnosticsTime = currentMillis;
        SystemManager::refreshDiagnostics(systemStatus);
        
        Serial.printf("[HEARTBEAT] State: %-7s (Threat: %-25s) | T: %.1fC | H: %.1f%% | Smk: %4u ADC | Flame: %s | WiFi: %s | GPS: %s (%u Sats) | Heap: %u B\n",
                      SystemManager::stateToString(systemStatus.currentState),
                      SystemManager::threatToString(systemStatus.activeThreat),
                      sensorData.temperature,
                      sensorData.humidity,
                      sensorData.smoke_raw,
                      sensorData.flame_detected ? "TRIGGERED!" : "Clear",
                      systemStatus.wifiConnected ? "Connected" : "Disconnected",
                      sensorData.gps_valid ? "FIX OK" : (gpsSensor.isHardwareConnected() ? "Searching" : "No Module"),
                      sensorData.gps_satellites,
                      systemStatus.freeHeapBytes);
    }
}
