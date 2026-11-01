#include "communication/web_server.h"
#include "system_manager.h"

DeviceWebServer::DeviceWebServer()
    : _server(80),
      _detection(nullptr),
      _emergency(nullptr),
      _latestSensors(nullptr),
      _latestStatus(nullptr) {
}

void DeviceWebServer::begin(DetectionController* detection, EmergencyController* emergency) {
    _detection = detection;
    _emergency = emergency;

    setupRoutes();
    _server.begin();
    Serial.println("[HTTP SERVER] REST API server listening on Port 80.");
}

void DeviceWebServer::sendCORSHeaders() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void DeviceWebServer::setupRoutes() {
    // Enable CORS pre-flight OPTIONS handler
    _server.on("/api/status", HTTP_OPTIONS, [this]() {
        sendCORSHeaders();
        _server.send(204);
    });

    _server.on("/api/emergency/reset", HTTP_OPTIONS, [this]() {
        sendCORSHeaders();
        _server.send(204);
    });

    _server.on("/api/thresholds", HTTP_OPTIONS, [this]() {
        sendCORSHeaders();
        _server.send(204);
    });

    // Core REST endpoints
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleGetStatus(); });
    _server.on("/api/emergency/reset", HTTP_POST, [this]() { handleResetEmergency(); });
    _server.on("/api/thresholds", HTTP_POST, [this]() { handleSetThresholds(); });

    _server.onNotFound([this]() { handleNotFound(); });
}

void DeviceWebServer::handleRoot() {
    sendCORSHeaders();
    String html = "<!DOCTYPE html><html><head><title>ESP32 Fire Detection Node</title></head>"
                  "<body style='font-family:sans-serif;padding:2rem;background:#111;color:#eee;'>"
                  "<h2>ESP32 Fire, Smoke & Temp Detection REST API</h2>"
                  "<p>Firmware v1.0.0-PROD | Status: Operational</p>"
                  "<ul>"
                  "<li><a style='color:#38bdf8;' href='/api/status'>GET /api/status</a> - Live JSON Telemetry</li>"
                  "<li>POST /api/emergency/reset - Disarm Emergency Latch</li>"
                  "<li>POST /api/thresholds - Dynamic Threshold Config</li>"
                  "</ul>"
                  "<p>Open the standalone Dashboard in <code>dashboard/index.html</code> to view live graphs.</p>"
                  "</body></html>";
    _server.send(200, "text/html", html);
}

void DeviceWebServer::handleGetStatus() {
    sendCORSHeaders();

    if (!_latestSensors || !_latestStatus) {
        _server.send(503, "application/json", "{\"error\":\"Telemetry pending initialization\"}");
        return;
    }

    JsonDocument doc;

    // 1. System Block
    JsonObject sys = doc["system"].to<JsonObject>();
    sys["state"] = SystemManager::stateToString(_latestStatus->currentState);
    sys["previous_state"] = SystemManager::stateToString(_latestStatus->previousState);
    sys["threat"] = SystemManager::threatToString(_latestStatus->activeThreat);
    sys["uptime_seconds"] = _latestStatus->uptimeSeconds;
    sys["free_heap"] = _latestStatus->freeHeapBytes;
    sys["reset_reason"] = SystemManager::getResetReasonString((esp_reset_reason_t)_latestStatus->resetReason);
    sys["wifi_connected"] = _latestStatus->wifiConnected;
    sys["ip_address"] = _latestStatus->ipAddress;
    sys["rssi"] = _latestStatus->wifiRSSI;

    // 2. Sensors Block
    JsonObject sensors = doc["sensors"].to<JsonObject>();
    sensors["temperature"] = serialized(String(_latestSensors->temperature, 1));
    sensors["humidity"] = serialized(String(_latestSensors->humidity, 0));
    sensors["smoke_raw"] = _latestSensors->smoke_raw;
    sensors["smoke_voltage"] = serialized(String(_latestSensors->smoke_voltage, 2));
    sensors["flame_detected"] = _latestSensors->flame_detected;
    sensors["emergency_pressed"] = _latestSensors->emergency_pressed;

    // Sensor Health Validation Flags
    JsonObject health = doc["health"].to<JsonObject>();
    health["dht_valid"] = _latestSensors->dht_valid;
    health["mq2_valid"] = _latestSensors->mq2_valid;
    health["flame_valid"] = _latestSensors->flame_valid;
    health["gps_valid"] = _latestSensors->gps_valid;

    // Geolocation Block
    JsonObject gps = doc["gps"].to<JsonObject>();
    gps["has_fix"] = _latestSensors->gps_valid;
    gps["latitude"] = serialized(String(_latestSensors->gps_latitude, 6));
    gps["longitude"] = serialized(String(_latestSensors->gps_longitude, 6));
    gps["satellites"] = _latestSensors->gps_satellites;
    gps["altitude_m"] = serialized(String(_latestSensors->gps_altitude_m, 1));
    gps["hdop"] = serialized(String(_latestSensors->gps_hdop, 1));
    gps["timestamp"] = _latestSensors->gps_time_str;

    if (_latestSensors->gps_valid) {
        char mapUrl[80];
        snprintf(mapUrl, sizeof(mapUrl), "https://maps.google.com/?q=%.6f,%.6f",
                 _latestSensors->gps_latitude, _latestSensors->gps_longitude);
        gps["maps_url"] = mapUrl;
    } else {
        gps["maps_url"] = "";
    }

    // Active Thresholds Block
    if (_detection) {
        JsonObject thresh = doc["thresholds"].to<JsonObject>();
        thresh["smoke_warn"] = _detection->getSmokeWarnThreshold();
        thresh["smoke_crit"] = _detection->getSmokeCritThreshold();
        thresh["temp_warn"] = _detection->getTempWarnThreshold();
        thresh["temp_crit"] = _detection->getTempCritThreshold();
    }

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void DeviceWebServer::handleResetEmergency() {
    sendCORSHeaders();
    if (_emergency) {
        _emergency->reset();
        Serial.println("[HTTP SERVER] Manual emergency reset executed via REST API.");
        _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Emergency disarmed successfully\"}");
    } else {
        _server.send(500, "application/json", "{\"error\":\"Emergency controller unavailable\"}");
    }
}

void DeviceWebServer::handleSetThresholds() {
    sendCORSHeaders();
    if (!_detection || !_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Invalid request or missing JSON body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"JSON parsing error\"}");
        return;
    }

    if (doc.containsKey("smoke_warn")) {
        _detection->setSmokeWarnThreshold(doc["smoke_warn"].as<uint16_t>());
    }
    if (doc.containsKey("smoke_crit")) {
        _detection->setSmokeCritThreshold(doc["smoke_crit"].as<uint16_t>());
    }
    if (doc.containsKey("temp_warn")) {
        _detection->setTempWarnThreshold(doc["temp_warn"].as<float>());
    }
    if (doc.containsKey("temp_crit")) {
        _detection->setTempCritThreshold(doc["temp_crit"].as<float>());
    }

    Serial.println("[HTTP SERVER] Sensor detection thresholds updated dynamically via REST API.");
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Thresholds updated\"}");
}

void DeviceWebServer::handleNotFound() {
    sendCORSHeaders();
    _server.send(404, "application/json", "{\"error\":\"Endpoint not found\"}");
}

void DeviceWebServer::handleClient(const SensorReadings& sensors, const SystemStatus& status) {
    _latestSensors = &sensors;
    _latestStatus = &status;
    _server.handleClient();
}

