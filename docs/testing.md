# System Integration Testing Specification & Verification Procedures

## 1. Testing Philosophy & Safety Protocols

> [!CAUTION]
> **LABORATORY SAFETY MANDATE:** Testing of this prototype must **NEVER** utilize uncontrolled open fires, hazardous smoke exposure, or combustible gas saturation. All test scenarios detailed below employ controlled, non-destructive, manufacturer-approved laboratory simulation methods.

This document details the test procedures, expected responses, and validation criteria for all **11 system operational test scenarios**.

---

## 2. Comprehensive Test Matrix

| Test ID | Test Scenario | Stimulation Method | Expected Local Output (LEDs / Buzzer / OLED) | Expected Remote Output (Telegram / Dashboard) |
|---|---|---|---|---|
| **TC-01** | **Normal Environmental Conditions** | Baseline ambient laboratory room air | Green LED **ON**, Red LED **OFF**, Buzzer **Silent**. OLED displays Page 0 nominal telemetry. | Dashboard shows `STATE_NORMAL`. Telegram sends single boot info; no alarms. |
| **TC-02** | **Elevated Temperature** | Gentle warm air stream from hair dryer directed at DHT11 | Green **OFF**, Red LED pulses (500ms), Buzzer chirps 150ms every 2s. OLED shows `WARN`. | Dashboard shows `STATE_WARNING`. Telegram sends single advisory warning. |
| **TC-03** | **Increased MQ-2 Smoke/Gas** | Controlled aerosol vapor / lower threshold in software | Green **OFF**, Red LED pulses (500ms), Buzzer chirps 150ms every 2s. OLED shows `WARN`. | Dashboard shows `STATE_WARNING`. Telegram sends single advisory warning. |
| **TC-04** | **Flame Sensor Activation** | Remote cigarette lighter flame (30 cm away) or IR emitter | Green **OFF**, Red LED rapid strobe (150ms), Buzzer continuous siren (200ms cadence). OLED flashes `FIRE CRITICAL`. | Dashboard shows `STATE_CRITICAL`. Immediate Telegram critical alert with GPS link. |
| **TC-05** | **Multiple Sensor Triggers** | Simultaneous warm air + aerosol vapor (Dual Threat) | Green **OFF**, Red LED rapid strobe, Buzzer continuous siren. OLED flashes `DUAL CRITICAL`. | Dashboard shows `STATE_CRITICAL`. Telegram sends immediate critical fire alert. |
| **TC-06** | **Emergency Button Activation** | Momentary press of push button on `GPIO 23` | Immediate latch: Green **OFF**, Red **ON**, Urgent alarm. OLED flashes `! MANUAL EMERGENCY !`. | Dashboard shows `STATE_EMERGENCY`. Immediate Telegram manual alert with GPS link. |
| **TC-07** | **GPS Fix Unavailable** | Disconnect GPS antenna or operate inside concrete building | System continues operating normally. OLED displays `GPS: Searching` or `Indoor`. | Telegram omits coordinates and indicates `Location Unavailable (Indoor Lab)`. |
| **TC-08** | **Wi-Fi Network Outage** | Turn off test Wi-Fi router or disconnect ESP32 antenna | Local alarms, Buzzer, LEDs, and OLED operate without delay. OLED shows `WF:--`. | Local alerts sound instantly. Background reconnect state machine retries every 10s. |
| **TC-09** | **Sensor Disconnection** | Temporarily disconnect DHT11 data pin or MQ-2 pin | Green slow toggle, Red diagnostic double-blink, Buzzer brief pip every 3s. OLED shows `FAULT`. | Dashboard shows `STATE_SENSOR_ERROR`. Telegram sends single hardware fault alert. |
| **TC-10** | **Telegram Delivery Failure** | Enter invalid bot token or disconnect internet gateway | Local alarm sounds immediately. System watchdog is fed and never locks up. | Failed HTTPS attempt logs to Serial; alert enqueued in ring buffer for retry. |
| **TC-11** | **Recovery After Abnormal Event**| Cease stimulation, allow room air to clear | Red LED turns **OFF**, Buzzer silences, Green LED illuminates solid **ON**. | Dashboard returns to `STATE_NORMAL`. Telegram sends single `[RECOVERY]` alert. |

---

## 3. Detailed Step-by-Step Test Procedures

### Test Case 1: Normal Environmental Conditions
* **Objective:** Verify stable baseline operation, sensor sampling, and absence of spurious false alarms.
* **Procedure:**
  1. Power on the ESP32 via a 5V/2A USB supply.
  2. Allow 30 seconds for initial MQ-2 heating coil stabilization.
  3. Observe the Serial Monitor, OLED display, and physical LEDs.
* **Pass Criteria:**
  * Green LED is illuminated solid.
  * Red LED is completely OFF.
  * Buzzer is silent.
  * OLED cycles between Page 0 and Page 1 with valid ambient readings ($20^\circ\text{C} \le T \le 35^\circ\text{C}$, $400 \le \text{Smoke} \le 750\text{ ADC}$).

---

### Test Case 2: Elevated Temperature Alone
* **Objective:** Verify single-sensor temperature pre-alarm warning and hysteresis without false fire evacuation.
* **Procedure:**
  1. Direct warm air at the DHT11 sensor from a distance of ~20 cm until temperature exceeds $45.0^\circ\text{C}$.
  2. Observe state change.
  3. Remove the warm air source and allow sensor to cool below $42.0^\circ\text{C}$ (hysteresis window).
* **Pass Criteria:**
  * System enters `STATE_WARNING` when $T \ge 45.0^\circ\text{C}$.
  * Buzzer emits intermittent 150 ms warning chirps (not full evacuation siren).
  * System does not return to normal until temperature drops below $42.0^\circ\text{C}$.

---

### Test Case 3: Increased Smoke/Gas Readings Alone
* **Objective:** Verify MQ-2 moving-average filter, smoke warning threshold, and noise rejection.
* **Procedure:**
  1. Introduce a safe test aerosol or temporarily lower `SMOKE_WARN_THRESHOLD_ADC` to baseline + 100 via the REST API.
  2. Observe moving average smooth ramp up to $\ge 1200\text{ ADC}$.
* **Pass Criteria:**
  * System transitions cleanly to `STATE_WARNING` (Reason: `SMOKE_LEVEL_ELEVATED`).
  * Red LED pulses; advisory Telegram message enqueued.

---

### Test Case 4: Optical Flame Sensor Trigger (Life-Safety Override)
* **Objective:** Verify that flame optical detection triggers instant critical evacuation regardless of other sensors.
* **Procedure:**
  1. Bring a cigarette lighter flame to approximately 30 cm in front of the flame photodiode for 1–2 seconds.
* **Pass Criteria:**
  * Immediate transition to `STATE_CRITICAL` within 100 ms.
  * Full evacuation alarm sounded: Buzzer toggles 200 ms ON / 200 ms OFF; Red LED strobes.
  * OLED flashes `FIRE CRITICAL` in full inverted video.
  * Immediate critical Telegram alert received on phone with active GPS link.
  * Flame persistence timer holds alarm active for at least 2.5 seconds after flame removal.

---

### Test Case 5: Dual-Threat Correlation (Smoke + Temperature)
* **Objective:** Verify sensor fusion rule: elevated smoke AND elevated temperature triggers confirmed `CRITICAL` alarm even without optical flame.
* **Procedure:**
  1. Exceed both $S \ge 1200\text{ ADC}$ and $T \ge 45.0^\circ\text{C}$ simultaneously.
* **Pass Criteria:**
  * System escalates from `WARNING` to `STATE_CRITICAL` (Reason: `EXTREME_SMOKE_AND_HEAT`).
  * Full audible alarm engages; critical Telegram alert dispatched.

---

### Test Case 6: Manual Emergency Button Latching & Reset
* **Objective:** Verify manual push button debouncing, break-glass latching, and 3-second long-press reset.
* **Procedure:**
  1. Tap the push button on `GPIO 23` momentarily ($< 200\text{ ms}$) and release.
  2. Verify system latches into `STATE_EMERGENCY`.
  3. Wait 10 seconds (alarm must continue sounding despite button released).
  4. Press and hold the button down continuously for 3.0 seconds.
* **Pass Criteria:**
  * Momentary tap triggers immediate latch into `STATE_EMERGENCY`.
  * Alarm remains active after release.
  * 3-second sustained hold cleanly disarms and silences the alarm.

---

### Test Case 7: GPS Unavailable (Indoor Fail-Safe)
* **Objective:** Verify that lack of satellite reception does not block fire detection, local alarms, or remote alerts.
* **Procedure:**
  1. Test ESP32 indoors where satellite signals cannot be locked.
  2. Trigger flame sensor.
* **Pass Criteria:**
  * Local alarms and sirens sound instantly.
  * Telegram alert sends: `Maps Link: Location Unavailable (Indoor Lab)`.
  * Zero coordinates (`0.0, 0.0`) are never presented as valid GPS coordinates.

---

### Test Case 8: Wi-Fi Disconnection & Network Outage
* **Objective:** Verify local life-safety independence during network failures.
* **Procedure:**
  1. Disconnect the Wi-Fi router.
  2. Trigger the emergency button.
* **Pass Criteria:**
  * Buzzer sounds immediately; Red LED turns ON; OLED flashes emergency screen without any delay.
  * CPU does not freeze or trigger watchdog timeout.
  * Upon restoring router, ESP32 reconnects automatically within 15 seconds.

---

### Test Case 9: Sensor Disconnection / Hardware Fault
* **Objective:** Verify sensor fault detection, CRC error trapping, and system stability.
* **Procedure:**
  1. Unplug the DHT11 data jumper wire from `GPIO 4`.
  2. Wait 6 seconds (3 consecutive failed read cycles).
* **Pass Criteria:**
  * System transitions to `STATE_SENSOR_ERROR` without crashing.
  * OLED shows `DHT: FAILED / DISCONNECTED`.
  * Reconnecting jumper wire restores `STATE_NORMAL` within 4 seconds.

---

### Test Case 10: Telegram Notification Delivery Failure
* **Objective:** Verify non-blocking HTTP timeout and watchdog resilience during internet loss.
* **Procedure:**
  1. Block internet access while keeping local Wi-Fi router alive.
  2. Trigger flame sensor.
* **Pass Criteria:**
  * Local buzzer and LED alarms fire immediately without hesitation.
  * HTTPS client times out cleanly after 3.5 seconds; watchdog is reset; main loop continues running smoothly.

---

### Test Case 11: Recovery After Abnormal Conditions
* **Objective:** Verify clean return to normal baseline and recovery alert generation.
* **Procedure:**
  1. Following any alarm event, restore ambient conditions.
* **Pass Criteria:**
  * State transitions from `CRITICAL` / `WARNING` back to `NORMAL`.
  * Buzzer silences, Red LED turns OFF, Green LED turns ON.
  * Single Telegram recovery message dispatched: `[RECOVERY] System Returned to Normal`.

