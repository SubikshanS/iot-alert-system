# Sensor Fusion & Threat Detection Algorithm Specification

## 1. Overview & Life-Safety Design Philosophy

The **IoT-Based Smart Fire, Smoke, and Temperature Detection System** employs a deterministic, rule-based sensor fusion algorithm. In mission-critical fire safety, statistical algorithms (such as black-box neural networks) cannot guarantee deterministic response times or fail-safe modes.

Our multi-sensor fusion model adheres to three core life-safety axioms:
1. **Immediate Override:** An optical flame signature or physical manual button press instantly escalates the system to a critical emergency, irrespective of whether other sensors have begun registering changes.
2. **Dual-Threat Escalation:** Moderate smoke alone or elevated ambient temperature alone triggers an advisory **WARNING** state. However, when both conditions occur concurrently, the engine fuses the correlation into a confirmed **CRITICAL** state.
3. **Anti-Flapping Hysteresis:** Environmental sensors experience turbulent fluctuations around threshold boundaries (e.g. rising smoke plumes). Dual-threshold hysteresis prevents erratic toggling between alarm states.

---

## 2. Threat Hierarchy & Decision Flow

```
+-----------------------------------------------------------------------------------+
|                        1. Manual Emergency Push Button?                           |
|                       YES ──► STATE_EMERGENCY (Instant)                           |
+-----------------------------------------------------------------------------------+
                                         │ NO
+-----------------------------------------------------------------------------------+
|                     2. Optical Infrared Flame Detected?                           |
|                        YES ──► STATE_CRITICAL (Instant)                           |
+-----------------------------------------------------------------------------------+
                                         │ NO
+-----------------------------------------------------------------------------------+
|       3. Extreme Smoke (>= 2200 ADC) OR Dual Elevated (Smoke >= 1200 & Temp >= 45°C)?|
|                       YES ──► STATE_CRITICAL (Alarm)                              |
+-----------------------------------------------------------------------------------+
                                         │ NO
+-----------------------------------------------------------------------------------+
|            4. Smoke >= 1200 ADC (Hysteresis 1050) OR Temp >= 45°C (Hysteresis 42°C)?|
|                       YES ──► STATE_WARNING (Advisory)                            |
+-----------------------------------------------------------------------------------+
                                         │ NO
+-----------------------------------------------------------------------------------+
|                   5. Sensor Disconnected / Electrical Fault?                      |
|                     YES ──► STATE_SENSOR_ERROR (Diagnostic)                       |
+-----------------------------------------------------------------------------------+
                                         │ NO
                                         ▼
                                   STATE_NORMAL
```

---

## 3. Mathematical Formulation & Hysteresis Damping

### 3.1 Hysteresis Damping Formulation
Let $S(t)$ be the 10-sample moving-average smoke ADC reading, and $T(t)$ be the validated temperature reading.

#### Smoke Warning State Hysteresis:
$$\text{Entry Condition: } S(t) \ge S_{\text{WARN}} \quad (1200\text{ ADC})$$
$$\text{Exit Condition: } S(t) < S_{\text{WARN}} - \Delta S_{\text{HYST}} \quad (1200 - 150 = 1050\text{ ADC})$$

#### Temperature Warning State Hysteresis:
$$\text{Entry Condition: } T(t) \ge T_{\text{WARN}} \quad (45.0^\circ\text{C})$$
$$\text{Exit Condition: } T(t) < T_{\text{WARN}} - \Delta T_{\text{HYST}} \quad (45.0^\circ\text{C} - 3.0^\circ\text{C} = 42.0^\circ\text{C})$$

#### Critical Dual-Threshold Hysteresis:
$$\text{Entry Condition: } S(t) \ge S_{\text{CRIT}} \quad (2200\text{ ADC}) \quad \lor \quad \Big(S(t) \ge S_{\text{WARN}} \ \land \ T(t) \ge T_{\text{WARN}}\Big)$$
$$\text{Exit Condition: } S(t) < S_{\text{CRIT}} - \Delta S_{\text{HYST}} \quad \land \quad \Big(S(t) < 1050\text{ ADC} \ \lor \ T(t) < 42.0^\circ\text{C}\Big)$$

---

## 4. Multi-Sensor Fusion Truth Table

| State # | Emergency Button | Flame DO | Smoke ADC ($S$) | Temp ($T$) | Sensor Valid? | Resulting System State | Primary Threat Reason |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **1** | **PRESSED** | Any | Any | Any | Any | **`STATE_EMERGENCY`** | `THREAT_MANUAL_EMERGENCY` |
| **2** | Released | **ACTIVE (LOW)** | Any | Any | Any | **`STATE_CRITICAL`** | `THREAT_FLAME_DETECTED` |
| **3** | Released | Inactive | $\ge 2200$ | Any | Valid | **`STATE_CRITICAL`** | `THREAT_DUAL_CRITICAL` |
| **4** | Released | Inactive | $\ge 1200$ | $\ge 45^\circ\text{C}$ | Valid | **`STATE_CRITICAL`** | `THREAT_DUAL_CRITICAL` |
| **5** | Released | Inactive | $\ge 1200$ | $< 45^\circ\text{C}$ | Valid | **`STATE_WARNING`** | `THREAT_SMOKE_WARN` |
| **6** | Released | Inactive | $< 1200$ | $\ge 45^\circ\text{C}$ | Valid | **`STATE_WARNING`** | `THREAT_TEMP_WARN` |
| **7** | Released | Inactive | $< 1050$ | $< 42^\circ\text{C}$ | **FAULT** | **`STATE_SENSOR_ERROR`**| `THREAT_SENSOR_FAULT` |
| **8** | Released | Inactive | $< 1050$ | $< 42^\circ\text{C}$ | **Valid** | **`STATE_NORMAL`** | `THREAT_NONE` |

---

## 5. Calibration Procedure for Physical Bench Testing

### A. MQ-2 Clean-Air Calibration
1. Power the system in a well-ventilated room free of smoke, alcohol, or solvent vapors.
2. Allow the sensor heating element to burn clean for at least 5–10 minutes.
3. Observe the baseline ADC count on the Serial Monitor (typically 450–650 ADC).
4. If your room's clean ambient baseline is significantly higher or lower, adjust `SMOKE_BASELINE_ADC` in `config.h`.
5. Suggested Warning Threshold: `Baseline + 600 ADC`.
6. Suggested Critical Threshold: `Baseline + 1600 ADC`.

### B. Temperature Threshold Calibration
1. Measure the baseline ambient temperature with the DHT11 (typically 24°C–30°C in laboratory conditions).
2. For demonstration purposes, `TEMP_WARN_THRESHOLD_C` is set to **45.0°C**, and `TEMP_CRIT_THRESHOLD_C` is set to **55.0°C**.
3. During safe demonstration, gentle heat from a hair dryer or cupped warm palm can be used to raise the sensor temperature above 45°C to demonstrate warning state transitions without hazardous open flames.

