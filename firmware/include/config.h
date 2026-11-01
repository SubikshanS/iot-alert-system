#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// GPIO PIN ASSIGNMENTS
// =============================================================================
// Analog Inputs (MUST be ADC1 channels to avoid Wi-Fi conflicts)
#define PIN_MQ2_ADC         34  // ADC1_CH6 (Input-only, safe with Wi-Fi)

// Digital Inputs
#define PIN_DHT11           4   // DHT11 1-Wire Single Bus Data
#define PIN_FLAME_DO        18  // Flame Sensor Digital Output (Active LOW)
#define PIN_EMERGENCY_BTN   23  // Manual Emergency Push Button (Active LOW, INPUT_PULLUP)

// UART2 for NEO-6M GPS Module
#define PIN_GPS_RX          16  // ESP32 RX2 <- GPS TX
#define PIN_GPS_TX          17  // ESP32 TX2 -> GPS RX
#define GPS_BAUD_RATE       9600

// I2C for 0.96" SSD1306 OLED Display
#define PIN_OLED_SDA        21  // Hardware I2C SDA
#define PIN_OLED_SCL        22  // Hardware I2C SCL
#define OLED_I2C_ADDR       0x3C
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64

// Actuator & Indicator Outputs
#define PIN_BUZZER          25  // Active Buzzer (Active HIGH)
#define PIN_LED_RED         26  // Red Warning/Alarm LED
#define PIN_LED_GREEN       27  // Green Normal Status LED

// =============================================================================
// SENSOR THRESHOLDS & HYSTERESIS
// =============================================================================
// Temperature Thresholds (Celsius)
#define TEMP_WARN_THRESHOLD_C       45.0f   // Warning state if temp >= 45°C
#define TEMP_CRIT_THRESHOLD_C       55.0f   // Critical alarm if combined with smoke
#define TEMP_HYSTERESIS_C           3.0f    // Clear warning when temp drops below (Warn - 3°C)

// MQ-2 Smoke & Gas Thresholds (12-bit ADC raw range: 0 - 4095)
// Calibrated with 2.2k / 3.3k voltage divider (3.0V max output at ADC)
#define SMOKE_BASELINE_ADC          600     // Typical clean air baseline reading
#define SMOKE_WARN_THRESHOLD_ADC    1200    // Warning state if smoke >= 1200
#define SMOKE_CRIT_THRESHOLD_ADC    2200    // Critical alarm if smoke >= 2200
#define SMOKE_HYSTERESIS_ADC        150     // Hysteresis window to prevent flapping

// MQ-2 Voltage Divider Factor: R2 / (R1 + R2) = 3.3k / (2.2k + 3.3k) = 0.60
#define MQ2_VOLTAGE_DIVIDER_RATIO   0.60f

// =============================================================================
// TASK SCHEDULING INTERVALS (in milliseconds)
// =============================================================================
#define INTERVAL_BUTTON_POLL_MS     20      // Button debouncing frequency
#define INTERVAL_FLAME_POLL_MS      50      // Flame sensor polling frequency
#define INTERVAL_MQ2_SAMPLE_MS      100     // MQ-2 ADC sampling (moving average)
#define INTERVAL_THREAT_EVAL_MS     100     // Sensor fusion & threat assessment
#define INTERVAL_OLED_REFRESH_MS    250     // Display screen refresh rate
#define INTERVAL_DHT11_SAMPLE_MS    2000    // DHT11 hardware limit is 1 - 2 seconds
#define INTERVAL_WIFI_CHECK_MS      5000    // Wi-Fi connection health check
#define INTERVAL_TELEGRAM_WORKER_MS 1000    // Telegram queue dispatch worker

// =============================================================================
// ALERT COOLDOWNS & RATE LIMITS (in milliseconds)
// =============================================================================
#define COOLDOWN_CRITICAL_ALERT_MS  30000   // 30 seconds between repeated critical alerts
#define COOLDOWN_WARNING_ALERT_MS   120000  // 2 minutes between repeated warning alerts

// =============================================================================
// SERIAL MONITOR & DEBUG
// =============================================================================
#define DEBUG_SERIAL_BAUD           115200

#endif // CONFIG_H

