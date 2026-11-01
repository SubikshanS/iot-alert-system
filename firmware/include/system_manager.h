#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"
#include <esp_task_wdt.h>
#include <esp_system.h>

class SystemManager {
public:
    static void initHardware();
    static void initWatchdog(uint32_t timeoutSeconds = 8);
    static void feedWatchdog();
    static void printBootDiagnostics();
    
    static const char* stateToString(SystemState state);
    static const char* threatToString(ThreatReason threat);
    static const char* getResetReasonString(esp_reset_reason_t reason);

    static void updateSystemUptime(SystemStatus& status);
    static void refreshDiagnostics(SystemStatus& status);
};

#endif // SYSTEM_MANAGER_H

