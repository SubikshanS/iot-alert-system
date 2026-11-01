#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "types.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

class WiFiManager {
public:
    WiFiManager();

    void begin();
    void checkConnection(SystemStatus& status);
    bool isConnected() const;

    String getLocalIP() const;
    int8_t getRSSI() const;
    String getSSID() const;

private:
    unsigned long _lastReconnectAttempt_ms;
    unsigned long _lastStatusCheck_ms;
    bool _wasConnected;
    uint8_t _reconnectAttempts;

    void updateStatus(SystemStatus& status);
    void attemptReconnect();
};

#endif // WIFI_MANAGER_H

