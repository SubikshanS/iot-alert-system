# Hardware Connections & Pinout Specification

## 1. System Overview & Electrical Architecture

The system utilizes an **ESP32 DevKit V1 (30-pin / 36-pin)** microcontroller operating on a 3.3V logic level. Several external peripherals require 5V supply rails (such as the MQ-2 heating coil and NEO-6M onboard low-dropout regulator), while the ESP32 input/output pins operate strictly at **3.3V max (absolute maximum: 3.6V)**.

> [!CAUTION]
> **ESP32 GPIOs are NOT 5V tolerant.** Connecting a 5V signal directly to any ESP32 GPIO pin will degrade or permanently destroy the ESP32 silicon input stage. All 5V analog and digital outputs must be level-shifted or conditioned through a voltage divider.

---

## 2. Complete GPIO Pin Assignment Table

| Pin Number | ESP32 GPIO | Peripheral Connected | Direction | Interface | Operating Voltage | Critical Constraints & Notes |
|---|---|---|---|---|---|---|
| **P1** | `GPIO 34` | **MQ-2 Smoke/Gas** | Input | ADC1 (CH6) | 0.0V - 3.0V (Conditioned) | **Must use ADC1.** ESP32 ADC2 cannot function when Wi-Fi is active. GPIO34 is input-only (no internal pullup/down). Fed via voltage divider. |
| **P2** | `GPIO 4` | **DHT11 Temp & Humidity** | In/Out | 1-Wire Single-Bus | 3.3V | Bidirectional digital pin. Requires 4.7kΩ to 10kΩ pull-up resistor to 3.3V (often integrated on 3-pin breakout modules). |
| **P3** | `GPIO 18` | **Flame Sensor (DO)** | Input | Digital Input | 3.3V | Connected to Digital Output of LM393 comparator on flame board. Active **LOW** when infrared flame radiation is detected. |
| **P4** | `GPIO 16` | **NEO-6M GPS (TX)** | Input | UART2 (RX2) | 3.3V | ESP32 Hardware Serial 2 Receive pin (`Serial2.begin(9600, SERIAL_8N1, 16, 17)`). |
| **P5** | `GPIO 17` | **NEO-6M GPS (RX)** | Output | UART2 (TX2) | 3.3V | ESP32 Hardware Serial 2 Transmit pin (optional configuration commands to GPS). |
| **P6** | `GPIO 21` | **OLED Display (SDA)** | In/Out | I2C Data | 3.3V | Hardware I2C data line. Connected to SSD1306 OLED (default address `0x3C`). |
| **P7** | `GPIO 22` | **OLED Display (SCL)** | Output | I2C Clock | 3.3V | Hardware I2C clock line. |
| **P8** | `GPIO 25` | **Active Buzzer** | Output | Digital Output | 3.3V / 5V | High logic activates buzzer alarm. Driven directly or through an NPN transistor buffer to avoid pin current overload. |
| **P9** | `GPIO 26` | **Red LED (Alarm)** | Output | Digital Output | 3.3V | Emergency/Warning indicator. Connected in series with 220Ω / 330Ω current-limiting resistor to GND. |
| **P10** | `GPIO 27` | **Green LED (Normal)**| Output | Digital Output | 3.3V | System healthy/Normal status indicator. Connected in series with 220Ω / 330Ω current-limiting resistor to GND. |
| **P11** | `GPIO 23` | **Emergency Push Button** | Input | Digital Input | 3.3V | Manual emergency activation. Configured with internal pull-up (`INPUT_PULLUP`), active **LOW** when pressed to GND. |

---

## 3. Critical Silicon & Electrical Considerations

### 3.1 ADC1 vs ADC2 Conflict with Wi-Fi
The ESP32 incorporates two Successive Approximation Register (SAR) Analog-to-Digital Converters:
- **ADC1**: GPIOs 32, 33, 34, 35, 36 (VP), 39 (VN).
- **ADC2**: GPIOs 0, 2, 4, 12, 13, 14, 15, 25, 26, 27.

The ESP32 Wi-Fi driver uses ADC2 internally for RF power calibration and temperature management. Whenever Wi-Fi is actively scanning, connecting, or transmitting packets, any call to `analogRead()` on an ADC2 pin will fail or return `0` / corrupted values.
- **Design Decision**: The MQ-2 analog output is strictly wired to **GPIO 34 (ADC1 Channel 6)**. This guarantees non-blocking, reliable analog sampling uninterrupted by Wi-Fi transmission bursts.

### 3.2 Boot Strapping Pins Avoidance
The ESP32 checks specific GPIO states during power-up or reset to determine boot mode:
- `GPIO 0`: Bootloader mode trigger (must be HIGH during normal boot).
- `GPIO 2`: Strapping pin (must be floating or LOW to enter UART download mode; has internal pull-down).
- `GPIO 12` (MTDI): Selects flash chip operating voltage (1.8V vs 3.3V). Forcing HIGH during boot can brown out flash.
- `GPIO 15`: Boot strapping / debug output.

**Design Decision**: None of our sensors, pull-downs, or switches are connected to GPIO 0, 2, 12, or 15. All sensors and buttons use neutral, safe pins.

---

## 4. Voltage Conditioning & Conditioning Calculations

### 4.1 MQ-2 Analog Output Voltage Divider
The MQ-2 sensor requires a 5.0V VCC supply because its internal tin-dioxide ($SnO_2$) heating element must maintain a thermal operating temperature of ~400°C to catalyze oxidation with combustible gases (LPG, Propane, Hydrogen, Methane, Smoke). As a result:
- The internal sensor resistance ($R_S$) forms a divider with the onboard load resistance ($R_L$).
- In high smoke or gas concentrations, the analog output voltage ($V_{AOUT}$) can swing close to 5.0V.
- If $V_{AOUT} = 5.0\text{V}$ is applied directly to ESP32 ADC, the pin clamp diodes will conduct, causing chip overheating and eventual ADC failure.

#### Voltage Divider Design
To scale $0.0\text{V} - 5.0\text{V}$ down to $0.0\text{V} - 3.0\text{V}$:
```
      MQ-2 AOUT (0 - 5.0V)
              │
             [R1] = 2.2 kΩ (or 10 kΩ)
              │
              ├───► Connected to ESP32 GPIO 34 (ADC1_CH6)
              │
             [R2] = 3.3 kΩ (or 20 kΩ)
              │
             GND
```

Using standard resistor values:
- $R_1 = 2.2\text{ k}\Omega$
- $R_2 = 3.3\text{ k}\Omega$

$$\text{Transfer Ratio } K = \frac{R_2}{R_1 + R_2} = \frac{3.3}{2.2 + 3.3} = \frac{3.3}{5.5} = 0.60$$

$$V_{\text{ADC\_MAX}} = 5.0\text{V} \times 0.60 = 3.00\text{V}$$

*Alternatively, with $R_1 = 10\text{ k}\Omega$ and $R_2 = 20\text{ k}\Omega$:*
$$K = \frac{20}{10 + 20} = 0.667 \implies V_{\text{ADC\_MAX}} = 5.0\text{V} \times 0.667 = 3.33\text{V}$$

Both configurations guarantee that the ESP32 ADC input never exceeds 3.3V. In firmware, the raw voltage is reconstructed by multiplying by $\frac{1}{K}$.

---

### 4.2 Active Buzzer Driving Considerations
An active buzzer contains an internal audio oscillator and produces a fixed resonant tone (~2.3 kHz) when powered with DC voltage.
- Typical current draw: **20 mA to 30 mA**.
- ESP32 GPIO pin rated output current: **12 mA max recommended** (absolute maximum 20 mA for short transients).
- Driving a 25 mA inductive buzzer load directly from an ESP32 GPIO pin will create thermal stress on the internal output driver FET and can lead to pin failure or micro-brownouts.

#### Driver Options:
1. **Option A (3-pin Active Buzzer Breakout Module)**: If using a breakout board with 3 pins (`VCC`, `GND`, `I/O`), it has an onboard S8050/S8550 NPN/PNP transistor. Connect `VCC` to 3.3V or 5V, `GND` to common ground, and `I/O` directly to `GPIO 25`.
2. **Option B (Bare 2-pin Buzzer Component)**: Use an external NPN transistor (e.g., 2N2222, BC547, or S8050):
   - Transistor Base connected to `GPIO 25` through a $1\text{ k}\Omega$ resistor.
   - Transistor Collector connected to Buzzer Negative (-).
   - Buzzer Positive (+) connected to 5V (or 3.3V).
   - Transistor Emitter connected to GND.
   - Place a flyback diode (e.g., 1N4148) in reverse bias across the buzzer terminals to suppress inductive voltage kicks when turned off.

---

### 4.3 LED Current-Limiting Resistors
- **ESP32 Output Voltage**: $V_{OH} = 3.3\text{ V}$.
- **Red LED**: Forward voltage $V_F \approx 1.8\text{ V}$ to $2.0\text{ V}$. Target current $I_D \approx 5\text{ mA}$ to $7\text{ mA}$.
  $$R_{\text{Red}} = \frac{3.3\text{V} - 1.9\text{V}}{0.006\text{A}} \approx 233\ \Omega \implies \mathbf{220\ \Omega \text{ or } 330\ \Omega}$$
  With $330\ \Omega$: $I = \frac{1.4\text{V}}{330\ \Omega} \approx 4.2\text{ mA}$ (sufficiently bright, highly efficient).
- **Green LED**: Forward voltage $V_F \approx 2.1\text{ V}$ to $2.4\text{ V}$. Target current $I_D \approx 4\text{ mA}$ to $6\text{ mA}$.
  $$R_{\text{Green}} = \frac{3.3\text{V} - 2.2\text{V}}{0.005\text{A}} \approx 220\ \Omega \implies \mathbf{220\ \Omega \text{ or } 330\ \Omega}$$

---

## 5. Power Budget & Distribution Plan

The system operates from a single regulated USB 5V power supply (minimum 5V / 2.0A recommended).

| Component | Operating Rail | Idle Current | Peak / Active Current | Notes |
|---|---|---|---|---|
| **ESP32 NodeMCU** | 3.3V (via onboard LDO) | 40 mA | 240 mA (bursts to 450 mA) | RF calibration and 802.11b/g/n packet bursts generate steep, transient current spikes. |
| **MQ-2 Heating Coil** | 5.0V (direct from VIN/USB) | 160 mA | 180 mA | Continuous heating duty cycle. Must NOT be powered from ESP32 3.3V pin (will overheat ESP32 LDO). |
| **NEO-6M GPS** | 5.0V (VCC to onboard LDO) | 45 mA | 67 mA | Peak during cold satellite acquisition and RF front-end lock. |
| **0.96" SSD1306 OLED**| 3.3V | 10 mA | 25 mA | Current proportional to illuminated pixel count. |
| **DHT11 Sensor** | 3.3V | 0.1 mA | 1.5 mA | Low duty cycle (~1 sec sample interval). |
| **Flame Sensor Board** | 3.3V | 2 mA | 5 mA | LM393 comparator circuit. |
| **Active Buzzer** | 5.0V or 3.3V | 0 mA | 25 mA | Active only during warning/critical alarm states. |
| **Status LEDs (Both)**| 3.3V | 0 mA | 10 mA | Current limited via 330Ω resistors. |
| **TOTAL** | — | **~257 mA** | **~763 mA (Peak)** | — |

### Power Supply Rules:
1. **Never power the MQ-2 heater from the ESP32 3.3V pin**: The ESP32 board's onboard LDO regulator (e.g., AMS1117-3.3 or ME6211) is typically rated for 500mA-800mA. The ESP32 itself can draw up to 450mA during Wi-Fi bursts. Adding 180mA of MQ-2 heater load will trigger thermal shutdown or cause a brownout reset (`Brownout detector was triggered`).
2. **MQ-2 VCC must be connected to the 5V / VIN rail** directly from the USB supply.
3. **Common Ground**: All ground pins (`GND` on ESP32, MQ-2, DHT11, Flame sensor, GPS, OLED, Buzzer, LEDs, and Button) must be tied together into a single, low-impedance ground bus.
4. **Decoupling Capacitor Recommendation**: Place a $470\ \mu\text{F}$ to $1000\ \mu\text{F}$ electrolytic capacitor across the 5V and GND rail on the breadboard, and a $0.1\ \mu\text{F}$ (100nF) ceramic capacitor across 3.3V and GND near the ESP32 to filter RF noise and prevent brownouts during Wi-Fi transmit bursts.

---

## 6. Wiring Matrix by Component

### 1. MQ-2 Gas & Smoke Sensor
- `VCC` ──► `ESP32 VIN` (5.0V USB Rail)
- `GND` ──► `ESP32 GND` (Common Ground)
- `AOUT` ──► `Voltage Divider Input` (R1: 2.2kΩ) ──► `ESP32 GPIO 34` (with R2: 3.3kΩ to GND)
- `DOUT` ──► *Not connected (analog readings provide granular PPM sensing)*

### 2. DHT11 Temperature & Humidity Sensor
- `VCC` ──► `ESP32 3V3` (3.3V Rail)
- `GND` ──► `ESP32 GND`
- `DATA` ──► `ESP32 GPIO 4` *(ensure 10kΩ pull-up between DATA and 3.3V if using 4-pin raw sensor)*

### 3. Flame Detection Sensor (LM393 3/4-pin module)
- `VCC` ──► `ESP32 3V3` (Powering from 3.3V ensures DO is 3.3V logic compatible!)
- `GND` ──► `ESP32 GND`
- `DO` (Digital Out) ──► `ESP32 GPIO 18`

### 4. NEO-6M GPS Module
- `VCC` ──► `ESP32 VIN` (5.0V - required by onboard LDO on NEO-6M module)
- `GND` ──► `ESP32 GND`
- `TXD` ──► `ESP32 GPIO 16` (RX2)
- `RXD` ──► `ESP32 GPIO 17` (TX2)

### 5. 0.96-inch I2C OLED Display (SSD1306, 128x64)
- `VCC` ──► `ESP32 3V3`
- `GND` ──► `ESP32 GND`
- `SCL` ──► `ESP32 GPIO 22`
- `SDA` ──► `ESP32 GPIO 21`

### 6. Active Buzzer Module
- `VCC` ──► `ESP32 3V3` or `VIN (5V)`
- `GND` ──► `ESP32 GND`
- `I/O` (Signal) ──► `ESP32 GPIO 25`

### 7. Status LEDs
- **Red LED**:
  - Anode (+) ──► `ESP32 GPIO 26`
  - Cathode (-) ──► `330Ω Resistor` ──► `ESP32 GND`
- **Green LED**:
  - Anode (+) ──► `ESP32 GPIO 27`
  - Cathode (-) ──► `330Ω Resistor` ──► `ESP32 GND`

### 8. Emergency Push Button
- Terminal A ──► `ESP32 GPIO 23`
- Terminal B ──► `ESP32 GND`
*(Software enables internal pull-up `INPUT_PULLUP`. Optional 100nF ceramic capacitor across terminals for hardware debouncing).*

---

## 7. System Wiring Diagram

```text
                               +---------------------------------------+
                               |              ESP32 DEVKIT             |
                               |                                       |
    [5V USB Rail] <---------- VIN                                      |
    [Common GND]  <---------- GND                                      |
                               |                                       |
                               | GPIO 34 (ADC1_CH6) <--- [2.2k / 3.3k Divider] <--- MQ-2 AOUT (5V)
                               | GPIO 4  (DHT11)    <--- DHT11 Data (3.3V)
                               | GPIO 18 (Flame DO) <--- Flame Sensor DO (3.3V)
                               | GPIO 16 (RX2)      <--- NEO-6M GPS TX
                               | GPIO 17 (TX2)      ---> NEO-6M GPS RX
                               | GPIO 21 (SDA)     <---> OLED SSD1306 SDA
                               | GPIO 22 (SCL)      ---> OLED SSD1306 SCL
                               | GPIO 25 (Buzzer)   ---> Buzzer Driver / Module
                               | GPIO 26 (Red LED)  ---> [330Ω] ---> Red LED Anode ---> GND
                               | GPIO 27 (Grn LED)  ---> [330Ω] ---> Green LED Anode -> GND
                               | GPIO 23 (Emerg Btn)<--- [Push Button] ---> GND
                               +---------------------------------------+
```

