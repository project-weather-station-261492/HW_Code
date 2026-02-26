#include "PowerManager.h"
#include "../ModemCore/ModemCore.h"

// ============================================
// POWER MANAGEMENT FUNCTIONS
// ============================================

/**
 * Initialize all relay control pins as digital outputs
 * Sets GPIO pins for relay control
 * Must be called during setup before using power control
 */
void initPowerPins() {
    Serial.println("[INIT] Relay pin");
    pinMode(SIM_PWR_RELAY_PIN,       OUTPUT);  // Relay 1: SIM COM
    pinMode(RS485_WINDSPEED_PWR_PIN, OUTPUT);  // Relay 2: Wind Speed
    pinMode(RS485_WINDDIR_PWR_PIN,   OUTPUT);  // Relay 3: Wind Direction
    pinMode(RS485_PVTEMP_PWR_PIN,    OUTPUT);  // Relay 4: PV Temp
    pinMode(RS485_BME_VEML_PWR_PIN,  OUTPUT);  // Relay 5: BME280 + VEML770
}

/**
 * Power on all devices (modem and sensors)
 * Activates all relay outputs to provide power
 * Waits 3 seconds for all devices to stabilize
 */
void PowerOnAll() {
    Serial.println("[PWR] Power ON all");
    digitalWrite(SIM_PWR_RELAY_PIN,       RELAY_ON);  // Relay 1: SIM COM
    digitalWrite(RS485_WINDSPEED_PWR_PIN, RELAY_ON);  // Relay 2: Wind Speed
    digitalWrite(RS485_WINDDIR_PWR_PIN,   RELAY_ON);  // Relay 3: Wind Direction
    digitalWrite(RS485_PVTEMP_PWR_PIN,    RELAY_ON);  // Relay 4: PV Temp
    digitalWrite(RS485_BME_VEML_PWR_PIN,  RELAY_ON);  // Relay 5: BME280 + VEML770
    delay(3000);
}

/**
 * Power off all devices (modem and sensors)
 * Gracefully shuts down modem first, then cuts power to all relays
 * Minimizes power consumption before deep sleep
 */
void PowerOffAll() {
    Serial.println("[PWR] Power OFF all");
    // sendATSimple("AT+CPOWD=1", 3000);  // Gracefully shutdown modem

    digitalWrite(SIM_PWR_RELAY_PIN,       RELAY_OFF);  // Relay 1: SIM COM
    digitalWrite(RS485_WINDSPEED_PWR_PIN, RELAY_OFF);  // Relay 2: Wind Speed
    digitalWrite(RS485_WINDDIR_PWR_PIN,   RELAY_OFF);  // Relay 3: Wind Direction
    digitalWrite(RS485_PVTEMP_PWR_PIN,    RELAY_OFF);  // Relay 4: PV Temp
    digitalWrite(RS485_BME_VEML_PWR_PIN,  RELAY_OFF);  // Relay 5: BME280 + VEML770
}

/**
 * Immediately cut power to all relays without attempting modem AT commands
 * Use this when modem is unresponsive to avoid recursion or further delays
 */
void PowerOffAllImmediate() {
    Serial.println("[PWR] Power OFF all (IMMEDIATE)");

    digitalWrite(SIM_PWR_RELAY_PIN,       RELAY_OFF);  // Relay 1: SIM COM
    digitalWrite(RS485_WINDSPEED_PWR_PIN, RELAY_OFF);  // Relay 2: Wind Speed
    digitalWrite(RS485_WINDDIR_PWR_PIN,   RELAY_OFF);  // Relay 3: Wind Direction
    digitalWrite(RS485_PVTEMP_PWR_PIN,    RELAY_OFF);  // Relay 4: PV Temp
    digitalWrite(RS485_BME_VEML_PWR_PIN,  RELAY_OFF);  // Relay 5: BME280 + VEML770
}
