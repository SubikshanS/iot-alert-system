#include "display/oled.h"
#include "system_manager.h"

OLEDDisplay::OLEDDisplay()
    : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1),
      _isInitialized(false),
      _currentPage(0),
      _lastPageSwitch_ms(0) {
}

bool OLEDDisplay::begin() {
    // Initialize I2C bus on GPIO21 (SDA) and GPIO22 (SCL)
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

    // Initialize SSD1306 OLED display at I2C address 0x3C
    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        Serial.println("[OLED] WARNING: SSD1306 allocation failed. Check I2C wiring (SDA/SCL) and address 0x3C.");
        _isInitialized = false;
        return false;
    }

    _isInitialized = true;
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);
    
    showBootSplash();
    Serial.println("[OLED] SSD1306 128x64 display initialized successfully.");
    return true;
}

void OLEDDisplay::showBootSplash() {
    if (!_isInitialized) return;

    _display.clearDisplay();
    _display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    _display.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, SSD1306_WHITE);

    _display.setTextSize(1);
    _display.setCursor(18, 14);
    _display.print("SMART FIRE SYSTEM");

    _display.setCursor(22, 28);
    _display.print("ESP32 IoT Node");

    _display.setCursor(16, 44);
    _display.print("Initializing...");

    _display.display();
}

void OLEDDisplay::nextPage() {
    _currentPage = (_currentPage + 1) % 2;
}

void OLEDDisplay::drawHeader(const char* title, const SystemStatus& status) {
    _display.fillRect(0, 0, SCREEN_WIDTH, 12, SSD1306_WHITE);
    _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    _display.setCursor(2, 2);
    _display.print(title);

    _display.setCursor(85, 2);
    if (status.wifiConnected) {
        _display.print("WF:OK");
    } else {
        _display.print("WF:--");
    }
    _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
}

void OLEDDisplay::renderPage0_Environmental(const SensorReadings& sensors, const SystemStatus& status) {
    char buf[32];
    drawHeader(SystemManager::stateToString(status.currentState), status);

    _display.setCursor(2, 16);
    snprintf(buf, sizeof(buf), "Temp: %.1f C  H: %.0f%%", sensors.temperature, sensors.humidity);
    _display.print(buf);

    _display.setCursor(2, 28);
    const char* smokeStatus = (sensors.smoke_raw >= SMOKE_CRIT_THRESHOLD_ADC) ? "CRIT" :
                              (sensors.smoke_raw >= SMOKE_WARN_THRESHOLD_ADC) ? "WARN" : "NORM";
    snprintf(buf, sizeof(buf), "Smoke: %4u (%s)", sensors.smoke_raw, smokeStatus);
    _display.print(buf);

    _display.setCursor(2, 40);
    snprintf(buf, sizeof(buf), "Flame: %s", sensors.flame_detected ? "!DETECTED!" : "Clear");
    _display.print(buf);

    _display.drawLine(0, 52, SCREEN_WIDTH, 52, SSD1306_WHITE);
    _display.setCursor(2, 55);
    if (sensors.gps_valid) {
        snprintf(buf, sizeof(buf), "GPS: Fix OK (%u Sat)", sensors.gps_satellites);
    } else {
        snprintf(buf, sizeof(buf), "GPS: Searching (%u)", sensors.gps_satellites);
    }
    _display.print(buf);
}

void OLEDDisplay::renderPage1_Connectivity(const SensorReadings& sensors, const SystemStatus& status) {
    char buf[32];
    drawHeader("SYSTEM STATUS", status);

    _display.setCursor(2, 16);
    if (status.wifiConnected) {
        snprintf(buf, sizeof(buf), "IP: %s", status.ipAddress);
    } else {
        snprintf(buf, sizeof(buf), "WiFi: Disconnected");
    }
    _display.print(buf);

    _display.setCursor(2, 28);
    if (sensors.gps_valid) {
        snprintf(buf, sizeof(buf), "Loc: %.4f, %.4f", sensors.gps_latitude, sensors.gps_longitude);
    } else {
        snprintf(buf, sizeof(buf), "GPS: No Fix (Indoor)");
    }
    _display.print(buf);

    _display.setCursor(2, 40);
    snprintf(buf, sizeof(buf), "Uptime: %lu s", (unsigned long)status.uptimeSeconds);
    _display.print(buf);

    _display.setCursor(2, 52);
    snprintf(buf, sizeof(buf), "Heap: %u KB", status.freeHeapBytes / 1024);
    _display.print(buf);
}

void OLEDDisplay::renderEmergencyScreen(const SensorReadings& sensors, const SystemStatus& status) {
    char buf[32];
    // Flashing visual inversion every 400ms for high-urgency visual cue
    bool invert = ((millis() / 400UL) % 2 == 0);

    if (invert) {
        _display.fillScreen(SSD1306_WHITE);
        _display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
        _display.fillScreen(SSD1306_BLACK);
        _display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }

    _display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, invert ? SSD1306_BLACK : SSD1306_WHITE);

    _display.setTextSize(1);
    _display.setCursor(16, 6);
    if (status.currentState == STATE_EMERGENCY) {
        _display.print("! MANUAL EMERGENCY !");
    } else {
        _display.print("!! FIRE CRITICAL !!");
    }

    _display.drawLine(4, 16, SCREEN_WIDTH - 4, 16, invert ? SSD1306_BLACK : SSD1306_WHITE);

    _display.setCursor(6, 22);
    snprintf(buf, sizeof(buf), "Cause: %s", SystemManager::threatToString(status.activeThreat));
    _display.print(buf);

    _display.setCursor(6, 34);
    snprintf(buf, sizeof(buf), "T: %.1fC | Smk: %u", sensors.temperature, sensors.smoke_raw);
    _display.print(buf);

    _display.setCursor(6, 46);
    if (sensors.gps_valid) {
        snprintf(buf, sizeof(buf), "GPS: %.4f, %.4f", sensors.gps_latitude, sensors.gps_longitude);
    } else {
        snprintf(buf, sizeof(buf), "GPS: Indoor / No Fix");
    }
    _display.print(buf);

    _display.setCursor(20, 55);
    _display.print("EVACUATE AREA");
}

void OLEDDisplay::update(const SensorReadings& sensors, const SystemStatus& status) {
    if (!_isInitialized) return;

    _display.clearDisplay();

    // High-priority emergency override: take over screen immediately
    if (status.currentState == STATE_CRITICAL || status.currentState == STATE_EMERGENCY) {
        renderEmergencyScreen(sensors, status);
    } else {
        // Auto-cycle pages every 4.0 seconds in normal or warning conditions
        if (millis() - _lastPageSwitch_ms >= 4000UL) {
            _lastPageSwitch_ms = millis();
            nextPage();
        }

        if (_currentPage == 0) {
            renderPage0_Environmental(sensors, status);
        } else {
            renderPage1_Connectivity(sensors, status);
        }
    }

    _display.display();
}

