#include "wifi_manager.h"

WiFiManager::WiFiManager()
    : _lastReconnectAttempt_ms(0),
      _lastStatusCheck_ms(0),
      _wasConnected(false),
      _reconnectAttempts(0) {
}

void WiFiManager::begin() {
    Serial.printf("[WIFI] Initializing Station Mode for SSID: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    // Initiate non-blocking connection request
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    _lastReconnectAttempt_ms = millis();

    // Give a short initial window (up to 2.5 seconds) without stalling boot
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 2500)) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        _wasConnected = true;
        Serial.printf("[WIFI] Connected immediately. IP Address: %s (RSSI: %d dBm)\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.println("[WIFI] Background connection in progress. Sensor monitoring will proceed autonomously.");
    }
}

void WiFiManager::updateStatus(SystemStatus& status) {
    if (WiFi.status() == WL_CONNECTED) {
        status.wifiConnected = true;
        strncpy(status.ipAddress, WiFi.localIP().toString().c_str(), sizeof(status.ipAddress) - 1);
        status.ipAddress[sizeof(status.ipAddress) - 1] = '\0';
        status.wifiRSSI = WiFi.RSSI();
    } else {
        status.wifiConnected = false;
        strcpy(status.ipAddress, "0.0.0.0");
        status.wifiRSSI = 0;
    }
}

void WiFiManager::attemptReconnect() {
    // Non-blocking reconnect attempt every 10 seconds
    if (millis() - _lastReconnectAttempt_ms >= 10000UL) {
        _lastReconnectAttempt_ms = millis();
        _reconnectAttempts++;
        Serial.printf("[WIFI] Reconnection attempt #%u to SSID: %s...\n", _reconnectAttempts, WIFI_SSID);
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
}

void WiFiManager::checkConnection(SystemStatus& status) {
    bool currentConnected = (WiFi.status() == WL_CONNECTED);

    if (currentConnected) {
        if (!_wasConnected) {
            _wasConnected = true;
            _reconnectAttempts = 0;
            Serial.printf("[WIFI] Connection Established! IP: %s (Signal: %d dBm)\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());
        }
    } else {
        if (_wasConnected) {
            _wasConnected = false;
            Serial.println("[WIFI] Connection dropped. Local alarms remain fully operational.");
        }
        attemptReconnect();
    }

    updateStatus(status);
}

bool WiFiManager::isConnected() const {
    return (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getLocalIP() const {
    return isConnected() ? WiFi.localIP().toString() : String("0.0.0.0");
}

int8_t WiFiManager::getRSSI() const {
    return isConnected() ? WiFi.RSSI() : 0;
}

String WiFiManager::getSSID() const {
    return String(WIFI_SSID);
}

