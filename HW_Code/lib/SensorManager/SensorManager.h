#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

// ============================================
// SENSOR CONFIGURATION
// ============================================
#define STATION_ID 1                    // Unique weather station identifier
#define CYCLES_PER_HOUR 4              // Number of 15-minute cycles in 1 hour (4 * 15min = 60min)

// ============================================
// SENSOR DATA FUNCTIONS
// ============================================
/**
 * Send hardware status report to API
 * Reports the status of all system components:
 * - SIM modem status
 * - All 4 RS485 device status
 * - Current cycle counter
 * JSON format: {"type":"hardware_status", "station_id", "cycle", "SIM", "RS1", "RS2", "RS3", "RS4"}
 * @param deep_sleep_cycle - Current cycle count from RTC memory
 */
void sendHW(uint32_t deep_sleep_cycle);

/**
 * Read and send sensor data from all RS485 devices
 * Collects:
 * - Solar temperature (ID 1, Reg 0)
 * - Wind direction number (ID 2, Reg 0)
 * - Wind direction (ID 2, Reg 1)
 * - Wind speed (ID 3, Reg 0)
 * - Air temperature (ID 4, Reg 0)
 * - Humidity (ID 4, Reg 1)
 * - Air pressure (ID 4, Reg 2)
 * - Light lux (ID 4, Reg 3)
 * Called once per hour (when hourly_cycle_count reaches CYCLES_PER_HOUR)
 * @param hourly_cycle_count - Reference to hour cycle counter (reset to 0 after send)
 */
void sendSensor(uint32_t& hourly_cycle_count);

// ============================================
// EXTERNAL STATE VARIABLES
// ============================================
extern char HW_sim_status[16];          // SIM modem status string
extern char HW_rs485_status[5][16];     // RS485 device status strings (indexed 1-4)

#endif
