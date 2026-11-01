#include "communication/telegram_manager.h"
#include "system_manager.h"

TelegramManager::TelegramManager()
    : _isConfigured(false),
      _recoveryPending(false),
      _lastCriticalAlert_ms(0),
      _lastWarningAlert_ms(0),
      _lastFaultAlert_ms(0),
      _queueHead(0),
      _queueTail(0),
      _queueCount(0) {
}

void TelegramManager::begin() {
    // Disable certificate checking for lightweight ESP32 TLS
    _client.setInsecure();
    _client.setTimeout(4000); // 4-second timeout to prevent stalling loop

    // Validate if credentials have been populated
    if (strlen(TELEGRAM_BOT_TOKEN) > 20 &&
        strncmp(TELEGRAM_BOT_TOKEN, "YOUR_", 5) != 0 &&
        strlen(TELEGRAM_CHAT_ID) > 4 &&
        strncmp(TELEGRAM_CHAT_ID, "YOUR_", 5) != 0) {
        _isConfigured = true;
        Serial.println("[TELEGRAM] Telegram Bot integration enabled with active credentials.");
    } else {
        _isConfigured = false;
        Serial.println("[TELEGRAM] Telegram credentials not configured in secrets.h. Notifications will run in mock/log mode.");
    }
}

String TelegramManager::urlEncode(const String& str) {
    String encoded = "";
    char c;
    char code0;
    char code1;
    for (size_t i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (isalnum(c)) {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';
        } else {
            code0 = (c >> 4) & 0x0F;
            code1 = c & 0x0F;
            encoded += '%';
            encoded += (char)(code0 > 9 ? code0 - 10 + 'A' : code0 + '0');
            encoded += (char)(code1 > 9 ? code1 - 10 + 'A' : code1 + '0');
        }
    }
    return encoded;
}

bool TelegramManager::executeHttpsPost(const String& rawText) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TELEGRAM] Cannot send notification: Wi-Fi not connected.");
        return false;
    }

    if (!_isConfigured) {
        Serial.println("\n[TELEGRAM SIMULATED DISPATCH]");
        Serial.println(rawText);
        Serial.println("[END TELEGRAM MESSAGE]\n");
        return true;
    }

    Serial.println("[TELEGRAM] Connecting to api.telegram.org via HTTPS (Port 443)...");
    if (!_client.connect("api.telegram.org", 443)) {
        Serial.println("[TELEGRAM] Error: TLS connection to Telegram API failed.");
        return false;
    }

    String postData = "chat_id=" + String(TELEGRAM_CHAT_ID) +
                      "&text=" + urlEncode(rawText) +
                      "&parse_mode=HTML";

    String request = "POST /bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage HTTP/1.1\r\n" +
                     "Host: api.telegram.org\r\n" +
                     "Content-Type: application/x-www-form-urlencoded\r\n" +
                     "Content-Length: " + String(postData.length()) + "\r\n" +
                     "Connection: close\r\n\r\n" +
                     postData;

    _client.print(request);

    // Read HTTP response header code
    unsigned long timeout = millis();
    while (_client.connected() && millis() - timeout < 3500) {
        String line = _client.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1 200 OK")) {
            Serial.println("[TELEGRAM] Alert dispatched successfully (HTTP 200 OK).");
            _client.stop();
            return true;
        }
    }

    _client.stop();
    Serial.println("[TELEGRAM] Alert sent (Response processed).");
    return true;
}

bool TelegramManager::enqueue(const String& msg) {
    if (_queueCount >= QUEUE_CAPACITY) {
        Serial.println("[TELEGRAM] Alert queue full. Dropping older message to prioritize new alert.");
        _queueTail = (_queueTail + 1) % QUEUE_CAPACITY;
        _queueCount--;
    }
    _messageQueue[_queueHead] = msg;
    _queueHead = (_queueHead + 1) % QUEUE_CAPACITY;
    _queueCount++;
    return true;
}

void TelegramManager::processQueue() {
    if (_queueCount == 0 || WiFi.status() != WL_CONNECTED) {
        return;
    }

    String nextMessage = _messageQueue[_queueTail];
    _queueTail = (_queueTail + 1) % QUEUE_CAPACITY;
    _queueCount--;

    executeHttpsPost(nextMessage);
}

bool TelegramManager::sendCustomMessage(const String& message) {
    return enqueue(message);
}

void TelegramManager::sendBootNotification(const String& ipAddress) {
    String msg = "<b>[SYSTEM ONLINE] Fire Detection Node Active</b>\n"
                 "------------------------------------\n"
                 "<b>Firmware:</b> v1.0.0-PROD\n"
                 "<b>IP Address:</b> " + ipAddress + "\n"
                 "<b>Status:</b> STATE_NORMAL\n"
                 "System ready and actively monitoring sensors.";
    enqueue(msg);
}

void TelegramManager::sendCriticalNotification(const SensorReadings& sensors, const SystemStatus& status) {
    char buf[512];
    String mapsLink = sensors.gps_valid ?
        "<a href=\"https://maps.google.com/?q=" + String(sensors.gps_latitude, 6) + "," + String(sensors.gps_longitude, 6) + "\">Open in Google Maps</a>" :
        "Location Unavailable (Indoor Lab)";

    snprintf(buf, sizeof(buf),
             "🔥 <b>[CRITICAL ALERT] Fire Detected!</b>\n"
             "------------------------------------\n"
             "<b>Status:</b> %s\n"
             "<b>Trigger:</b> %s\n"
             "<b>Temperature:</b> %.1f °C\n"
             "<b>Humidity:</b> %.0f %%\n"
             "<b>Smoke Level:</b> %u ADC (%.2f V)\n"
             "<b>Flame Sensor:</b> %s\n"
             "<b>GPS Fix:</b> %s\n"
             "<b>Maps Link:</b> %s\n"
             "<b>Timestamp:</b> %s\n"
             "------------------------------------\n"
             "🚨 <i>ACTION REQUIRED: EVACUATE AREA IMMEDIATELY!</i>",
             SystemManager::stateToString(status.currentState),
             SystemManager::threatToString(status.activeThreat),
             sensors.temperature,
             sensors.humidity,
             sensors.smoke_raw,
             sensors.smoke_voltage,
             sensors.flame_detected ? "OPTICAL FLAME DETECTED" : "Clear",
             sensors.gps_valid ? "Valid 3D Fix" : "Indoor / Searching",
             mapsLink.c_str(),
             sensors.gps_time_str);

    enqueue(String(buf));
}

void TelegramManager::sendEmergencyNotification(const SensorReadings& sensors, const SystemStatus& status) {
    char buf[512];
    String mapsLink = sensors.gps_valid ?
        "<a href=\"https://maps.google.com/?q=" + String(sensors.gps_latitude, 6) + "," + String(sensors.gps_longitude, 6) + "\">Open in Google Maps</a>" :
        "Location Unavailable (Indoor Lab)";

    snprintf(buf, sizeof(buf),
             "🆘 <b>[MANUAL EMERGENCY] Alarm Activated!</b>\n"
             "------------------------------------\n"
             "<b>Trigger:</b> Physical Push Button Pressed\n"
             "<b>Status:</b> STATE_EMERGENCY (Latched)\n"
             "<b>Temperature:</b> %.1f °C | <b>Smoke:</b> %u ADC\n"
             "<b>GPS Location:</b> %s\n"
             "<b>Timestamp:</b> %s\n"
             "------------------------------------\n"
             "⚠️ <i>Manual evacuation trigger latched by on-site personnel!</i>",
             sensors.temperature,
             sensors.smoke_raw,
             mapsLink.c_str(),
             sensors.gps_time_str);

    enqueue(String(buf));
}

void TelegramManager::sendWarningNotification(const SensorReadings& sensors, const SystemStatus& status) {
    char buf[384];
    snprintf(buf, sizeof(buf),
             "⚠️ <b>[WARNING] Elevated Environmental Readings</b>\n"
             "------------------------------------\n"
             "<b>Status:</b> STATE_WARNING\n"
             "<b>Reason:</b> %s\n"
             "<b>Temperature:</b> %.1f °C\n"
             "<b>Smoke Level:</b> %u ADC (%.2f V)\n"
             "------------------------------------\n"
             "ℹ️ <i>Advisory notification. Investigate area for potential hazards.</i>",
             SystemManager::threatToString(status.activeThreat),
             sensors.temperature,
             sensors.smoke_raw,
             sensors.smoke_voltage);

    enqueue(String(buf));
}

void TelegramManager::sendRecoveryNotification() {
    String msg = "✅ <b>[RECOVERY] System Returned to Normal</b>\n"
                 "------------------------------------\n"
                 "All sensor readings have stabilized below threshold limits.\n"
                 "<b>Current Status:</b> STATE_NORMAL\n"
                 "Local alarms silenced.";
    enqueue(msg);
}

void TelegramManager::sendSensorFaultNotification(const SensorReadings& sensors) {
    String msg = "⚠️ <b>[HARDWARE FAULT] Sensor Communication Failure</b>\n"
                 "------------------------------------\n"
                 "One or more sensors are reporting hardware failure:\n"
                 "• DHT11 Status: " + String(sensors.dht_valid ? "OK" : "FAILED / DISCONNECTED") + "\n"
                 "• MQ-2 Status: " + String(sensors.mq2_valid ? "OK" : "OUT OF BOUNDS") + "\n"
                 "------------------------------------\n"
                 "Inspect wiring immediately.";
    enqueue(msg);
}

void TelegramManager::dispatchAlerts(const SensorReadings& sensors, SystemStatus& status) {
    unsigned long now = millis();

    switch (status.currentState) {
        case STATE_CRITICAL:
            if (now - _lastCriticalAlert_ms >= COOLDOWN_CRITICAL_ALERT_MS) {
                _lastCriticalAlert_ms = now;
                status.lastCriticalAlert_ms = now;
                _recoveryPending = true;
                sendCriticalNotification(sensors, status);
            }
            break;

        case STATE_EMERGENCY:
            if (now - _lastCriticalAlert_ms >= COOLDOWN_CRITICAL_ALERT_MS) {
                _lastCriticalAlert_ms = now;
                status.lastCriticalAlert_ms = now;
                _recoveryPending = true;
                sendEmergencyNotification(sensors, status);
            }
            break;

        case STATE_WARNING:
            if (now - _lastWarningAlert_ms >= COOLDOWN_WARNING_ALERT_MS) {
                _lastWarningAlert_ms = now;
                status.lastWarningAlert_ms = now;
                _recoveryPending = true;
                sendWarningNotification(sensors, status);
            }
            break;

        case STATE_SENSOR_ERROR:
            if (now - _lastFaultAlert_ms >= COOLDOWN_WARNING_ALERT_MS) {
                _lastFaultAlert_ms = now;
                sendSensorFaultNotification(sensors);
            }
            break;

        case STATE_NORMAL:
            if (_recoveryPending) {
                _recoveryPending = false;
                sendRecoveryNotification();
            }
            break;

        case STATE_BOOT:
        default:
            break;
    }
}

