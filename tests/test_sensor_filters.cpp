#include <iostream>
#include <cstdint>
#include <cassert>
#include <cmath>

// Standalone filter logic verification test
class MovingAverageFilter {
public:
    static const uint8_t SIZE = 10;
    uint16_t buffer[SIZE] = {0};
    uint8_t index = 0;
    uint8_t count = 0;
    uint32_t sum = 0;

    void update(uint16_t val) {
        if (count < SIZE) {
            buffer[index] = val;
            sum += val;
            count++;
            index = (index + 1) % SIZE;
        } else {
            sum -= buffer[index];
            buffer[index] = val;
            sum += val;
            index = (index + 1) % SIZE;
        }
    }

    uint16_t getAverage() const {
        return (count == 0) ? 0 : (sum / count);
    }
};

int main() {
    std::cout << "[TEST] Running Sensor Filter & Logic Verification...\n";

    // 1. Test Moving Average Filter
    MovingAverageFilter filter;
    for (int i = 0; i < 10; i++) {
        filter.update(1000);
    }
    assert(filter.getAverage() == 1000);
    std::cout << "  [PASS] Steady-state moving average (1000 ADC) == 1000\n";

    // Add a single transient spike (noise glitch)
    filter.update(4000);
    // New average should be (9 * 1000 + 4000) / 10 = 13000 / 10 = 1300
    assert(filter.getAverage() == 1300);
    std::cout << "  [PASS] Single noise spike smoothed: 4000 spike dampened to 1300\n";

    // 2. Test Voltage Divider Reconstruction
    const float RATIO = 0.60f;
    uint16_t adcRaw = 2048; // ~midpoint (1.65V at pin)
    float pinVoltage = ((float)adcRaw / 4095.0f) * 3.3f;
    float reconstructedVoltage = pinVoltage / RATIO;
    // Expected: (1.65 / 0.60) = ~2.75V
    assert(std::abs(reconstructedVoltage - 2.75f) < 0.05f);
    std::cout << "  [PASS] Voltage reconstruction: 2048 ADC -> " << reconstructedVoltage << " V (Expected ~2.75V)\n";

    // 3. Test Disconnection / Bounds checking
    auto isValidADC = [](uint16_t raw) {
        return (raw >= 40 && raw <= 4085);
    };
    assert(isValidADC(0) == false);       // Disconnected / floating GND
    assert(isValidADC(25) == false);      // Out of bounds
    assert(isValidADC(4090) == false);    // Shorted to rail
    assert(isValidADC(650) == true);      // Normal clean air
    assert(isValidADC(2200) == true);     // Heavy smoke
    std::cout << "  [PASS] Electrical sanity bounds correctly reject 0, 25, and 4090 ADC\n";

    // 4. Test Flame Logic Inversion
    auto isFlameActive = [](int pinLevel) {
        return (pinLevel == 0); // Active LOW
    };
    assert(isFlameActive(0) == true);  // Low = Fire
    assert(isFlameActive(1) == false); // High = No Fire
    std::cout << "  [PASS] Flame sensor active-low conversion verified\n";

    std::cout << "[TEST] All Sensor Logic Tests Passed Successfully!\n";
    return 0;
}

