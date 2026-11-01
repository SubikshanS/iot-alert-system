#include "system_manager.h"
#include <esp_idf_version.h>

void SystemManager::initHardware() {
    // 1. Configure Output Actuators and Indicators
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    pinMode(PIN_LED_RED, OUTPUT);
    digitalWrite(PIN_LED_RED, LOW);

    pinMode(PIN_LED_GREEN, OUTPUT);
    digitalWrite(PIN_LED_GREEN, HIGH); // Default to green active on initial power-up

    // 2. Configure Digital Input Sensors & Buttons
    pinMode(PIN_FLAME_DO, INPUT);
    pinMode(PIN_EMERGENCY_BTN, INPUT_PULLUP); // Active LOW with internal pull-up

    // 3. Configure ADC for MQ-2 Analog Input
    pinMode(PIN_MQ2_ADC, INPUT);
    analogReadResolution(12); // Standard 12-bit resolution: 0 - 4095
    analogSetPinAttenuation(PIN_MQ2_ADC, ADC_11db); // 11dB attenuation gives full 0 - 3.1V dynamic range
}

void SystemManager::initWatchdog(uint32_t timeoutSeconds) {
#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = timeoutSeconds * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&wdt_config);
    esp_task_wdt_add(NULL); // Subscribe the Arduino loopTask
#else
    esp_task_wdt_init(timeoutSeconds, true);
    esp_task_wdt_add(NULL);
#endif
    Serial.printf("[WDT] Task Watchdog Timer configured: %u seconds timeout (Panic: Enabled)\n", timeoutSeconds);
}

void SystemManager::feedWatchdog() {
    esp_task_wdt_reset();
}

const char* SystemManager::getResetReasonString(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:   return "Power-on reset";
        case ESP_RST_EXT:       return "External pin reset";
        case ESP_RST_SW:        return "Software reset via esp_restart";
        case ESP_RST_PANIC:     return "Software panic / exception";
        case ESP_RST_INT_WDT:   return "Interrupt watchdog reset";
        case ESP_RST_TASK_WDT:  return "Task watchdog reset";
        case ESP_RST_WDT:       return "Other watchdog reset";
        case ESP_RST_DEEPSLEEP: return "Wakeup from deep sleep";
        case ESP_RST_BROWNOUT:  return "Brownout reset (Voltage supply dipped below threshold)";
        case ESP_RST_SDIO:      return "Reset over SDIO";
        default:                return "Unknown / unclassified reset";
    }
}

void SystemManager::printBootDiagnostics() {
    esp_reset_reason_t resetReason = esp_reset_reason();

    Serial.println("\n=======================================================");
    Serial.println("  IoT-Based Smart Fire, Smoke & Temp Detection System  ");
    Serial.println("  Firmware: v1.0.0-PROD | Platform: ESP-WROOM-32       ");
    Serial.println("=======================================================");
    Serial.printf("  CPU Frequency     : %u MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  Flash Memory Size : %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("  Free Heap Memory  : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("  Min Free Heap Ever: %u bytes\n", ESP.getMinFreeHeap());
    Serial.printf("  ESP-IDF Version   : %s\n", esp_get_idf_version());
    Serial.printf("  Reset Cause       : [%d] %s\n", (int)resetReason, getResetReasonString(resetReason));
    Serial.println("-------------------------------------------------------");
    Serial.println("  Peripherals Pin Mapping:");
    Serial.printf("  - MQ-2 ADC1 (CH6) : GPIO %d (Conditioned 0-3.0V)\n", PIN_MQ2_ADC);
    Serial.printf("  - DHT11 Data      : GPIO %d\n", PIN_DHT11);
    Serial.printf("  - Flame Sensor DO : GPIO %d\n", PIN_FLAME_DO);
    Serial.printf("  - Emergency Button: GPIO %d (Active LOW)\n", PIN_EMERGENCY_BTN);
    Serial.printf("  - Active Buzzer   : GPIO %d\n", PIN_BUZZER);
    Serial.printf("  - Red / Green LEDs: GPIO %d / GPIO %d\n", PIN_LED_RED, PIN_LED_GREEN);
    Serial.printf("  - I2C (SDA/SCL)   : GPIO %d / GPIO %d\n", PIN_OLED_SDA, PIN_OLED_SCL);
    Serial.printf("  - GPS UART2 (RX/TX): GPIO %d / GPIO %d\n", PIN_GPS_RX, PIN_GPS_TX);
    Serial.println("=======================================================\n");
}

const char* SystemManager::stateToString(SystemState state) {
    switch (state) {
        case STATE_BOOT:         return "BOOT";
        case STATE_NORMAL:       return "NORMAL";
        case STATE_WARNING:      return "WARNING";
        case STATE_CRITICAL:     return "CRITICAL";
        case STATE_EMERGENCY:    return "EMERGENCY";
        case STATE_SENSOR_ERROR: return "SENSOR_ERROR";
        default:                 return "UNKNOWN";
    }
}

const char* SystemManager::threatToString(ThreatReason threat) {
    switch (threat) {
        case THREAT_NONE:             return "NONE";
        case THREAT_SMOKE_WARN:       return "SMOKE_LEVEL_ELEVATED";
        case THREAT_TEMP_WARN:        return "TEMPERATURE_ELEVATED";
        case THREAT_FLAME_DETECTED:   return "OPTICAL_FLAME_DETECTED";
        case THREAT_DUAL_CRITICAL:    return "EXTREME_SMOKE_AND_HEAT";
        case THREAT_MANUAL_EMERGENCY: return "MANUAL_PUSH_BUTTON";
        case THREAT_SENSOR_FAULT:     return "SENSOR_COMMUNICATION_FAULT";
        default:                      return "UNKNOWN";
    }
}

void SystemManager::updateSystemUptime(SystemStatus& status) {
    status.uptimeSeconds = millis() / 1000UL;
}

void SystemManager::refreshDiagnostics(SystemStatus& status) {
    status.freeHeapBytes = ESP.getFreeHeap();
    status.resetReason = (uint8_t)esp_reset_reason();
    updateSystemUptime(status);
}

