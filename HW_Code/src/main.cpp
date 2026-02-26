#include <Arduino.h>
#include <ModbusRTUMaster.h>
#include <esp_task_wdt.h>

// ================= MODULAR LIBRARIES =================
#include <ModemCore.h>          // Modem AT commands and network
#include <HttpClient.h>         // HTTP data transmission
#include <PowerManager.h>       // Power relay control
#include <RS485Handler.h>       // Modbus RTU sensor communication
#include <SensorManager.h>      // Sensor data collection

// ================= PIN DEFINITIONS =================
// These pins are used across multiple modules
#define MODEM_RX 16             // Modem UART RX
#define MODEM_TX 17             // Modem UART TX
#define DE_PIN 27               // RS485 driver enable pin

// ================= SYSTEM CONFIGURATION =================
#define SLEEP_INTERVAL_SECONDS 60*15  // Deep sleep duration (15 minutes)
// #define SLEEP_INTERVAL_SECONDS 60  // Deep sleep duration (1 minutes for testing)
#define WDT_TIMEOUT 90              // Watchdog timer timeout (90 seconds)
#define WDT_FEED() esp_task_wdt_reset()  // Macro to feed watchdog timer

// ================= HARDWARE INTERFACES =================
ModbusRTUMaster modbus(Serial1, DE_PIN);  // Modbus RTU master for RS485
HardwareSerial Modem(2);                   // UART2 for cellular modem

// ================= RTC PERSISTENT VARIABLES =================
// Preserved across deep sleep cycles
RTC_DATA_ATTR uint32_t deep_sleep_cycle = 0;      // Total boot counter
RTC_DATA_ATTR uint32_t hourly_cycle_count = 0;    // 10-minute cycle counter (resets at 6)

// ================= SYSTEM STATE VARIABLES =================
uint32_t modem_fail_count = 0;      // Cumulative modem connection failures
char HW_sim_status[16] = "Unknown"; // SIM card status

// ============================================
// MAIN SETUP - CALLED ON EVERY BOOT
// ============================================
/**
 * System initialization and sensor data collection
 * Execution sequence:
 * 1. Initialize serial, watchdog, power system
 * 2. Power on all devices and modems
 * 3. Initialize communication interfaces (Modem, RS485)
 * 4. Check device status (SIM, RS485 slaves)
 * 5. Send hardware status report
 * 6. Collect and send sensor data (once per hour)
 * 7. Power down all peripherals
 * 8. Enter deep sleep for 10 minutes
 * Total execution time: ~30-60 seconds
 * Deep sleep current: <5mA
 */
void setup() {

    Serial.begin(115200);
    delay(1000);

    Serial.println("\n==============================");
    Serial.println("ESP32 BOOT");
    Serial.println("==============================");

    // ========== Initialize Watchdog ==========
    // Prevents system hang if code loops unexpectedly
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);

    // ========== Initialize Power Management ==========
    // Setup relay pins and power on all devices
    initPowerPins();
    PowerOnAll();

    // ========== Initialize Modems ==========
    Serial.println("[INIT] Modem UART");
    Modem.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    delay(6000);  // Allow modem to stabilize

    // ========== Print Modem Firmware Version ==========
    String gmrResp;
    sendAT("AT+GMR", gmrResp, 3000);
    Serial.println("[MODEM] Firmware: " + gmrResp);

    // ========== Initialize RS485 ==========
    initRS485();

    // ========== Check SIM Status ==========
    String resp;
    bool sim_ok = false;
    // Retry up to 5 times; if still unresponsive, power off everything
    for (int retry = 0; retry < 5; retry++) {
        Serial.printf("[SIM] AT try %d\n", retry + 1);
        if (sendAT("AT", resp, 2000)) {
            strcpy(HW_sim_status, "Normal");
            Serial.println("[SIM] OK");
            sim_ok = true;
            break;
        }
        Serial.println("[SIM] FAIL");
        delay(500);
        WDT_FEED();
    }

    if (!sim_ok) {
        strcpy(HW_sim_status, "Error");
        Serial.println("[FATAL] Modem unresponsive after 5 retries. Shutting down.");
        // Immediately cut power to peripherals and stop operation
        PowerOffAllImmediate();
        Serial.println("[SLEEP] Enter deep sleep to halt operation");
        esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_INTERVAL_SECONDS * 1000000ULL);
        Serial.flush();
        esp_deep_sleep_start();
    }

    // ========== Test RS485 Devices ==========
    testRS485();

    // ========== Increment Cycle Counter ==========
    // Persists across deep sleep in RTC memory
    deep_sleep_cycle++;
    Serial.printf("[CYCLE] %u\n", deep_sleep_cycle);

    // ========== Send Hardware Status ==========
    // Sent every boot (every 10 minutes)
    sendHW(deep_sleep_cycle);

    // ========== Send Sensor Data (Hourly) ==========
    // Accumulates 4 cycles (4 * 15min = 60min), then sends sensor data
    if (hourly_cycle_count >= CYCLES_PER_HOUR) {
        Serial.println("[CYCLE] Hourly sensor send");
        sendSensor(hourly_cycle_count);
        hourly_cycle_count = 0;
    }
    hourly_cycle_count++;

    // ========== Power Down ==========
    PowerOffAll();

    // ========== Enter Deep Sleep ==========
    // Wakes up after SLEEP_INTERVAL_SECONDS and reboots completely
    Serial.println("[SLEEP] Enter deep sleep...");
    esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_INTERVAL_SECONDS * 1000000ULL);
    Serial.flush();
    esp_deep_sleep_start();
}

// ============================================
// LOOP - NOT USED (Deep sleep resets MCU)
// ============================================
void loop() {}
