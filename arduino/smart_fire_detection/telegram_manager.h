#ifndef TELEGRAM_MANAGER_H
#define TELEGRAM_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "types.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

class TelegramManager {
public:
    TelegramManager();

    void begin();
    
    /**
     * @brief Evaluates current system state and dispatches rate-limited Telegram notifications.
     */
    void dispatchAlerts(const SensorReadings& sensors, SystemStatus& status);

    /**
     * @brief Low-frequency worker to send queued messages non-blockingly.
     */
    void processQueue();

    bool sendCustomMessage(const String& message);
    void sendBootNotification(const String& ipAddress);

    bool isConfigured() const { return _isConfigured; }

private:
    WiFiClientSecure _client;
    bool _isConfigured;
    bool _recoveryPending;

    unsigned long _lastCriticalAlert_ms;
    unsigned long _lastWarningAlert_ms;
    unsigned long _lastFaultAlert_ms;

    // Small message ring buffer
    static const uint8_t QUEUE_CAPACITY = 4;
    String _messageQueue[QUEUE_CAPACITY];
    uint8_t _queueHead;
    uint8_t _queueTail;
    uint8_t _queueCount;

    bool enqueue(const String& msg);
    bool executeHttpsPost(const String& encodedText);
    String urlEncode(const String& str);

    void sendCriticalNotification(const SensorReadings& sensors, const SystemStatus& status);
    void sendEmergencyNotification(const SensorReadings& sensors, const SystemStatus& status);
    void sendWarningNotification(const SensorReadings& sensors, const SystemStatus& status);
    void sendRecoveryNotification();
    void sendSensorFaultNotification(const SensorReadings& sensors);
};

#endif // TELEGRAM_MANAGER_H

