#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "types.h"
#include "detection.h"
#include "emergency.h"

class DeviceWebServer {
public:
    DeviceWebServer();

    void begin(DetectionController* detection, EmergencyController* emergency);
    void handleClient(const SensorReadings& sensors, const SystemStatus& status);

private:
    WebServer _server;
    DetectionController* _detection;
    EmergencyController* _emergency;

    const SensorReadings* _latestSensors;
    const SystemStatus*   _latestStatus;

    void setupRoutes();
    void handleRoot();
    void handleGetStatus();
    void handleResetEmergency();
    void handleSetThresholds();
    void handleNotFound();
    void sendCORSHeaders();
};

#endif // WEB_SERVER_H

