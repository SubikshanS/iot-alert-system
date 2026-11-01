#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "types.h"

class OLEDDisplay {
public:
    OLEDDisplay();

    bool begin();
    void update(const SensorReadings& sensors, const SystemStatus& status);
    void showBootSplash();
    void nextPage();

    bool isInitialized() const { return _isInitialized; }

private:
    Adafruit_SSD1306 _display;
    bool _isInitialized;
    uint8_t _currentPage;
    unsigned long _lastPageSwitch_ms;

    void renderPage0_Environmental(const SensorReadings& sensors, const SystemStatus& status);
    void renderPage1_Connectivity(const SensorReadings& sensors, const SystemStatus& status);
    void renderEmergencyScreen(const SensorReadings& sensors, const SystemStatus& status);
    void drawHeader(const char* title, const SystemStatus& status);
};

#endif // OLED_DISPLAY_H

