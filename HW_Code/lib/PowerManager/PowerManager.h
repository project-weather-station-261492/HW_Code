#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

// ============================================
// POWER RELAY PIN DEFINITIONS
// ============================================
// Relay 1 — SIM COM (Modem)
#define SIM_PWR_RELAY_PIN       32      // Relay 1: SIM modem power control
// Relay 2–5 — RS485 Sensors
#define RS485_WINDSPEED_PWR_PIN 13      // Relay 2: Wind Speed (SlaveID=1)
#define RS485_WINDDIR_PWR_PIN   14      // Relay 3: Wind Direction (SlaveID=2)
#define RS485_PVTEMP_PWR_PIN    18      // Relay 4: PV Temp / PT100 (SlaveID=3)
#define RS485_BME_VEML_PWR_PIN  19      // Relay 5: BME280 + VEML770 (SlaveID=4)

// ============================================
// RELAY LOGIC LEVELS
// ============================================
#define RELAY_ON HIGH                    // Logic level to activate relay (pull to ground)
#define RELAY_OFF LOW                  // Logic level to deactivate relay (float high)

// ============================================
// POWER MANAGEMENT FUNCTIONS
// ============================================
/**
 * Initialize all power relay pins as outputs
 * Must be called once during setup before using power functions
 */
void initPowerPins();

/**
 * Turn on power to all devices
 * Enables:
 * - SIM modem power
 * - All 4 RS485 device power supplies
 * Includes 3-second stabilization delay
 */
void PowerOnAll();

/**
 * Turn off power to all devices
 * Process:
 * 1. Send power-down command to modem (AT+CPOWD=1)
 * 2. Disable all relay outputs
 * Used before deep sleep to minimize power consumption
 */
void PowerOffAll();

/**
 * Power off all devices immediately without attempting modem shutdown
 * Useful when modem is unresponsive to avoid calling AT commands
 */
void PowerOffAllImmediate();

#endif
