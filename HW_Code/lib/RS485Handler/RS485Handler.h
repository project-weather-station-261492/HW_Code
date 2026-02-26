#ifndef RS485_HANDLER_H
#define RS485_HANDLER_H

#include <Arduino.h>
#include <ModbusRTUMaster.h>

// ============================================
// RS485 SERIAL CONFIGURATION
// ============================================
#define RX1 25                          // RS485 RX pin on ESP32
#define TX1 26                          // RS485 TX pin on ESP32
#define DE_PIN 27                       // Driver Enable pin for RS485 transceiver

// ============================================
// RS485 HANDLER FUNCTIONS
// ============================================
/**
 * Initialize RS485 serial communication
 * Configures UART1 for Modbus RTU protocol
 * Baud rate: 9600, 8N1 (8 bits, no parity, 1 stop bit)
 */
void initRS485();

/**
 * Test all four RS485 slave devices
 * Reads holding register 0 from each slave (ID 1-4)
 * Updates HW_rs485_status with "Normal" or "Error" for each device
 * Useful for diagnostics during startup
 */
void testRS485();

/**
 * Read a register value from an RS485 slave device
 * Attempts up to 3 reads with 200ms delay between attempts
 * @param id - Modbus slave ID (1-4)
 * @param reg - Register address to read
 * @param val - Pointer to uint16_t to store read value
 * @return true if read successful, false if all 3 attempts failed
 */
bool readReg(int id, int reg, uint16_t* val);

// ============================================
// SLAVE 4 (BME280 + VEML770) — FC4 BLOCK READ
// ============================================

/**
 * Holds decoded data from Slave 4 (5 input registers, FC4)
 * All raw values are pre-scaled at device side:
 *   temperature : raw / 100.0 => °C
 *   humidity    : raw / 100.0 => %
 *   pressure    : raw / 10.0  => hPa
 *   lux         : raw (uint16_t)
 * ok_bme / ok_veml reflect status bits from Address 0
 */
struct Slave4Data {
    bool    ok_bme;         // true = BME280 healthy (bit0 of status == 0)
    bool    ok_veml;        // true = VEML770 healthy (bit1 of status == 0)
    uint16_t temperature;   // raw ×100 (divide by 100 for °C)
    uint16_t humidity;      // raw ×100 (divide by 100 for %)
    uint16_t pressure;      // raw ×10  (divide by 10 for hPa)
    uint16_t lux;           // raw lux value
};

/**
 * Read all Slave 4 input registers in one FC4 request (Addr 0, Qty 5)
 * Retries up to 3 times with 200ms delay between attempts
 * @param out - Pointer to Slave4Data struct to populate
 * @return true if read successful, false if all retries failed
 */
bool readSlave4(Slave4Data* out);

// ============================================
// EXTERNAL STATE VARIABLES
// ============================================
extern char HW_rs485_status[5][16];     // Status string for each slave (indexed 1-4)
extern ModbusRTUMaster modbus;          // Modbus RTU master object

// Slave 4 sensor status (set by testRS485, based on Status register bit0/bit1)
extern bool HW_s4_bme_ok;              // true = BME280 healthy (bit0 == 0)
extern bool HW_s4_veml_ok;             // true = VEML770 healthy (bit1 == 0)


#endif
